# Wheel Simulator
## Setup
1. Install and build Project Chrono's [DEM-Engine](https://github.com/projectchrono/DEM-Engine). 
2. Clone this directory.
3. In `CMakeLists.txt`, update `DEME_DIR` to point to the path where you built the DEM Engine.
4. In `main.cpp`, update `wheel_filepath` and `terrain_filepath` to point to an appropriate wheel mesh and terrain file.
5. In `main.cpp`, update `data_drivepath` to point to the top-level directory in which you wish to store your data
6. If you have not yet, create a build folder within the directory: `mkdir build`
7. Build this code:
```bash
cd build
cmake ..
make
```
## Running
Run the simulation using the following command:
```bash
./WheelSimulator <slip_value> <sim_endtime> <batch_name>
# for example, ./WheelSimulator 0.2 5.0 demo_batch
```
Pass in the slip value and sim endtime as doubles, and the batch name as a string.