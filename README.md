# PQC Kyber - HW/SW Accelerator on PYNQ-Z2 Board

## Overview

This repository contains the design and implementation of a hardware/software co-design accelerator for the post-quantum cryptography (PQC) algorithm Kyber. The project uses Vitis HLS (High-Level Synthesis) to accelerate critical Kyber operations on an PYNQ-Z2, integrating the generated hardware IP with the control software through an AXI interface.

The repository supports and provides designs for three security levels: Kyber-512, Kyber-768, and Kyber-1024.

## Repository Structure

The repository is divided into three main directories:

1. **`integrated_kyber/`**: Contains the integrated version of the system (Hardware + Software).


2. **`standalone_accel/`**: Contains the HLS synthesizable C/C++ code, pure software versions for comparison, and pre-compiled bitstreams.


3. **`vivado/`**: Contains the complete Xilinx Vivado projects (Block Designs and Processing System integrations).



---

### 1. `integrated_kyber/` (HW + SW Integration)



This folder contains the complete Kyber implementation developed to offload heavy operations to the FPGA.

* **`hw_wrapper.c` and `hw_wrapper.h**`: These files are responsible for the communication bridge. They configure data transfers (DMA/AXI) and send commands from the ARM processor to the FPGA.


* **Kyber Components (`indcpa.c`, `kem.c`, `polyvec.c`, etc.)**: Original Kyber C files, modified at exact points to trigger hardware wrappers instead of pure software execution.


* **`test/` and `nistkat/**`: Test suite (Known Answer Tests) and speed benchmarks (`test_speed.c`, `test_kyber.c`) to validate that the accelerated version returns the exact same mathematical results as the standard version.


* **`Makefile`**: Automation file to compile the application on the processor (ARM) side.


* **`Benchmark.ipynb`**: Jupyter Notebook created to easily run integration and performance tests directly on boards based on the PYNQ framework.



### 2. `standalone_accel/` (HLS Accelerator and Bitstreams)



Folder that contains the isolated development of the IP Core and storage of FPGA binaries.

* **`polyvec_accel.c` and `polyvec_accel.h**`: Contain the acceleration logic. These files include Vitis HLS directives/pragmas (`#pragma HLS ...`) to create pipelines, unroll loops, and define array partitioning, transforming the C code into hardware.


* **`bitstream_files/`**: Folder containing the `.bit` (FPGA configuration) and `.hwh` (Hardware Handoff) files for the three variants (`1024`, `768`, `512`). These are required to instantiate the hardware using PYNQ.


* **`polyvec_sw/`**: A software copy of the behavior (`polyvec.c`, `poly.c`, `ntt.c`, etc.) to run comparative analyses against the hardware (Golden Reference).


* **`Benchmark.ipynb`**: Testing notebook focused on validating the transfer and execution latencies of the isolated IP Core before full Kyber integration.



### 3. `vivado/` (Vivado Hardware Projects)



Complete system architecture connecting the HLS IP Core to the Zynq Processing System.

* **`polyvec_keccak_accel_512/`, `polyvec_keccak_accel_768/`, `polyvec_keccak_accel_1024/**`: Vivado project directories segregated by Kyber level.


* Each folder contains the base project files (`.xpr`) and Block Design descriptions (`.bd`, `.bda`, `.ui`) under the `srcs/sources_1/bd/polyvec_accel/` path.


* **`ip/`**: Contains the instantiation of logic blocks such as `polyvec_accel_0_0` (the exported HLS IP), AXI interconnect networks (`axi_smc`, `axi_mem_intercon`), and the ARM `processing_system7` core itself.



---

## Requirements and Dependencies

* Xilinx Zynq-7000 or Zynq UltraScale+ SoC FPGA-based board (e.g., PYNQ-Z1, PYNQ-Z2, Kria).
* PYNQ ecosystem image running on the board.
* (Optional) Xilinx Vivado and Vitis HLS if you wish to modify the architecture and recompile bitstreams.

## How to Run (Deployment via PYNQ)

1. **File Transfer:**
Upload the `integrated_kyber` and `standalone_accel` folders into your Jupyter environment running on PYNQ (connected via network).


2. **Standalone Accelerator Test:**
* Navigate to the `standalone_accel` directory.


* Open the `Benchmark.ipynb` file.


* The notebook will internally call the files located in `bitstream_files/` to overlay the FPGA configuration and test input/output vectors against the functions in `polyvec_sw/`.




3. **Integrated Kyber Test (Co-design):**
* Navigate to the `integrated_kyber` directory.


* If you prefer using the terminal, you can run `make` to build the executables.


* Otherwise, open the `Benchmark.ipynb` in this folder. It will automatically configure the hardware, initialize the interface between PS and PL (via `hw_wrapper`), and perform a complete benchmark generating keys, encapsulating, and decapsulating (KEM), highlighting the hardware performance gains.





## How to Recompile Hardware

If you modify the HLS codes in `standalone_accel/polyvec_accel.c`:

1. Import the C files into Vitis HLS, rerun C Synthesis, and Export RTL.
2. Open the desired project version (`.xpr`) located in the `vivado/` folder using the Xilinx Vivado GUI.


3. Update the IP repository to capture your new HLS design in `polyvec_accel_0_0` and validate the Block Design.


4. Run "Generate Bitstream".
5. Copy the generated `.bit` file and the `.hwh` file to the `standalone_accel/bitstream_files/` folder, overwriting the original versions to reflect the new logic in the PYNQ notebook.
