# Wheel Simulator
## Setup
1. Install and build Project Chrono's [DEM-Engine](https://github.com/projectchrono/DEM-Engine). 
2. Clone this directory.
3. In `CMakeLists.txt`, update `DEME_DIR` to point to the path where you built the DEM Engine.
4. Create a build folder within the directory: `mkdir build`
5. Build this code:
```bash
cd build
cmake ..
make
```
## Running
Run the simulation using the following command:
```bash
./WheelSimulator <slip> <sim_endtime> <batch_dir_name> <wheel_path> <terrain_path> <data_path>
# for example, ./WheelSimulator 0.2 5.0 demo_batch /path/to/wheel.obj /path/to/terrain.csv /path/to/output/dir/
```
Pass in the slip value and sim endtime as doubles, and the batch name as a string.
