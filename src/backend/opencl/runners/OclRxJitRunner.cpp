/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "backend/opencl/runners/OclRxJitRunner.h"

#include "backend/opencl/cl/rx/randomx_run_gfx803.h"
#include "backend/opencl/cl/rx/randomx_run_gfx900.h"
#include "backend/opencl/kernels/rx/Blake2bHashRegistersKernel.h"
#include "backend/opencl/kernels/rx/HashAesKernel.h"
#include "backend/opencl/kernels/rx/RxJitKernel.h"
#include "backend/opencl/kernels/rx/RxRunKernel.h"
#include "backend/opencl/OclLaunchData.h"
#include "backend/opencl/wrappers/OclLib.h"
#include "backend/opencl/wrappers/OclError.h"


xmrig::OclRxJitRunner::OclRxJitRunner(size_t index, const OclLaunchData &data) : OclRxBaseRunner(index, data)
{
}


xmrig::OclRxJitRunner::~OclRxJitRunner()
{
    delete m_randomx_jit;
    delete m_randomx_run;

    OclLib::release(m_asmProgram);
    OclLib::release(m_intermediate_programs);
    OclLib::release(m_programs);
    OclLib::release(m_registers);
}


void xmrig::OclRxJitRunner::build()
{
    OclRxBaseRunner::build();

    const uint32_t batch_size = data().thread.intensity();

    m_hashAes1Rx4->setArgs(m_scratchpads, m_registers, 256, batch_size);
    m_blake2b_hash_registers_32->setArgs(m_hashes, m_registers, 256);
    m_blake2b_hash_registers_64->setArgs(m_hashes, m_registers, 256);

    m_randomx_jit = new RxJitKernel(m_program);
    m_randomx_jit->setArgs(m_entropy, m_registers, m_intermediate_programs, m_programs, batch_size, m_rounding);

    if (!loadAsmProgram()) {
        throw std::runtime_error(OclError::toString(CL_INVALID_PROGRAM));
    }

    m_randomx_run = new RxRunKernel(m_asmProgram);
    m_randomx_run->setArgs(data().dataset->get(), m_scratchpads, m_registers, m_rounding, m_programs, batch_size, m_algorithm);
}


void xmrig::OclRxJitRunner::execute(uint32_t iteration)
{
    const uint32_t g_intensity = data().thread.intensity();

    m_randomx_jit->enqueue(m_queue, g_intensity, iteration);

    OclLib::finish(m_queue);

    m_randomx_run->enqueue(m_queue, g_intensity);
}


void xmrig::OclRxJitRunner::init()
{
    OclRxBaseRunner::init();

    const size_t g_thd = data().thread.intensity();

    m_registers             = OclLib::createBuffer(m_ctx, CL_MEM_READ_WRITE, 256 * g_thd, nullptr);
    m_intermediate_programs = OclLib::createBuffer(m_ctx, CL_MEM_READ_WRITE, 5120 * g_thd, nullptr);
    m_programs              = OclLib::createBuffer(m_ctx, CL_MEM_READ_WRITE, 10048 * g_thd, nullptr);
}


bool xmrig::OclRxJitRunner::loadAsmProgram()
{
    // Adrenaline drivers on Windows and amdgpu-pro drivers on Linux use ELF header's flags (offset 0x30) to store internal device ID
    // Read it from compiled OpenCL code and substitute this ID into pre-compiled binary to make sure the driver accepts it
    uint32_t elf_header_flags = 0;
    const uint32_t elf_header_flags_offset = 0x30;

    size_t bin_size;
    if (OclLib::getProgramInfo(m_program, CL_PROGRAM_BINARY_SIZES, sizeof(bin_size), &bin_size) != CL_SUCCESS) {
        return false;
    }

    std::vector<char> binary_data(bin_size);
    char* tmp[1] = { binary_data.data() };
    if (OclLib::getProgramInfo(m_program, CL_PROGRAM_BINARIES, sizeof(char*), tmp) != CL_SUCCESS) {
        return false;
    }

    if (bin_size >= elf_header_flags_offset + sizeof(uint32_t)) {
        elf_header_flags = *reinterpret_cast<uint32_t*>((binary_data.data() + elf_header_flags_offset));
    }

    const size_t len      = (m_gcn_version == 14) ? randomx_run_gfx900_bin_size : randomx_run_gfx803_bin_size;
    unsigned char *binary = (m_gcn_version == 14) ? randomx_run_gfx900_bin      : randomx_run_gfx803_bin;

    // Set correct internal device ID in the pre-compiled binary
    if (elf_header_flags) {
        *reinterpret_cast<uint32_t*>(binary + elf_header_flags_offset) = elf_header_flags;
    }

    cl_int status;
    cl_int ret;
    cl_device_id device = data().device.id();

    m_asmProgram = OclLib::createProgramWithBinary(ctx(), 1, &device, &len, (const unsigned char**) &binary, &status, &ret);
    if (ret != CL_SUCCESS) {
        return false;
    }

    return OclLib::buildProgram(m_asmProgram, 1, &device) == CL_SUCCESS;
}