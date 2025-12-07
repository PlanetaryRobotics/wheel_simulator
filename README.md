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
module load cuda/11.7.1
module load gcc/10.2.0
make -j 64
```
6. Configure parameters: inside input files, there are 3 JSON files. In `job_parameters.json`, setup the input and output directories, as well as the simulation endtime. To use the example file, remember to replace the user name. In `terrain_parameters.json` and `wheel_parameters.json`, configure the corresponding parameters as needed
7. Setup dependencies for wheel optimizer script: 
```bash
chmod +x setup_env.sh
./setup_env.sh
```
8. To run optimizer that replace the original wheel.obj file, change desired parameters in `wheel_parameters.json` and run:
```bash
source .venv/bin/activate
python gen_wheel.py
```
Please note that the newly generated obj file goes into the path stored in `job_parameters.json`

## Running
Run the simulation using the following command:
```bash
./WheelSimulator <input_json_folder_path> <batch_name> <slip>
# for example, ./WheelSimulator /path/to/input_files "demo_batch" 0.0
```
Pass in the slip value as doubles, and the batch name as a string.

To run on the moonshot-chrono machine:
Use Document/wheel_simulator already cloned and refer to the `job_parameters_chrono.json` file. Remember to change the file name to job_parameters.json. Please refer to `wheel_terrain_chrono` folder to find the output files. Run locally using this command:
```bash
./WheelSimulator "input_files" <batch_name> <slip>
```

To run multiple simulations on PSC, use this script:
```bash
#!/bin/bash

#SBATCH --nodes 1                         
#SBATCH --partition GPU-shared
#SBATCH --gpus=v100-32:2
#SBATCH --mail-type=ALL

#SBATCH --output output_20251102-%A_%a.out
#SBATCH --error error_log_20251102-%A_%a.err

# ----↓--- Params to Modify ------↓------


#SBATCH --time 24:00:00		# max job time, make sure it doesn't cut off your sim early!
#SBATCH --array=0-2			# array range corresponds with size of SLIPS vector

BATCH_NAME="BATCH_NAME"		# add a descriptive name here. Does not affect functionality
SIM_ENDTIME=10.0				# in seconds, how long to run the wheel for
SLIPS=(0.0 0.15 0.3)			# the range of slip values evaluated in the sim

BIN_DIR="${HOME}/moonranger_mobility/wheel_simulator/build" # Path to where you built the code

INPUT_JSON="${HOME}/moonranger_mobility/wheel_simulator/input_files" # Path to folder of all three json files

# ----↑---------------------------↑-----


echo "Loading modules..."

module load cuda/11.7.1
module load gcc/10.2.0

echo "Loaded modules."

echo "Navigating to target directory: ${BIN_DIR}"

cd ${BIN_DIR}

echo "Starting Sim"

job_id=$SLURM_JOB_ID
array_id=$SLURM_ARRAY_TASK_ID
echo "job ID ${job_id} array ID ${array_id}"

SLIP=${SLIPS[${array_id}]}
echo "Slip for array ID ${array_id} is ${SLIP}"

echo "Sim inputs: ${INPUT_JSON} ${BATCH_NAME} ${SLIP}"
./WheelSimulator ${INPUT_JSON} ${BATCH_NAME} ${SLIP}
echo "Sim complete"

```

## Note:
1. Here are the meanings of the fields in material properties in `wheel_parameters.json` and `terrain_parameters.json` :
```json
    "material_properties": {
      "E": 1000000000.0,    // Young's modulus
      "nu": 0.3,            // Poisson's ratio
      "CoR": 0.3,           // Coefficient of restitution
      "mu": 0.5,            // Friction coefficient
      "Crr": 0.0            // Rolling resistance coefficient
    }
```

2. The following fields for `wheel_parameters.json` are not passed into Wheel structure, so they are deleted for this version. They might be needed in the pygalmesh branch and can be added back there.
```json
{
    "grouser_number": 18,
    "grouser_height": 0.01,
    "control_point_deviation": 0.3,
}
```
