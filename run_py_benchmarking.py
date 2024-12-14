import sys
import os
# sys.path.append(os.path.abspath("build/python"))
sys.path.append(os.path.abspath("build"))
sys.path.append('scripts')
from map import Map
import json
import random

map_path="/mnt/home/ruthluvu/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/maps/symbotic_medium.map"
# full_weight_path="scripts/random_weight_001.w"
with_wait_costs=True

map=Map(map_path)
map.print_graph(map.graph)

def write_into_file(filename, my_list):
    with open(filename, 'w') as file:
        file.write(f"{len(my_list)}\n")
    
        for item in my_list:
            file.write(f"{item}\n")

def get_matching_coordinates(file_path, character, char_list = []):
    coordinates = []

    with open(file_path, 'r') as file:
        lines = file.readlines()

        # Start reading the map data after the "map" line
        map_data_start = False
        for row_index, line in enumerate(lines):
            if map_data_start:
                for col_index, char in enumerate(line.strip()):
                    if char == character or char in char_list:
                        #breakpoint()
                        coordinates.append(len(line.strip())*(row_index - map_start_row)+col_index)
            
            if "map" in line:
                map_data_start = True
                map_start_row = row_index +1
    
    return coordinates

spawn_locs = get_matching_coordinates(map_path, "r", ["o", "i", "."])
inbound_locs = get_matching_coordinates(map_path, "i")
aisles_locs = get_matching_coordinates(map_path, "a")
outbound_locs = get_matching_coordinates(map_path, "a")
extra_spawn_locs = get_matching_coordinates(map_path, "o")

breakpoint()
write_into_file("/mnt/home/ruthluvu/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/symbotic_medium_spawn.agents", spawn_locs)
write_into_file("/mnt/home/ruthluvu/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/symbotic_medium_inbound.agents", inbound_locs)
write_into_file("/mnt/home/ruthluvu/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/symbotic_medium_outbound.agents", outbound_locs)
write_into_file("/mnt/home/ruthluvu/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/symbotic_medium_aisles.agents", aisles_locs)

random.shuffle(spawn_locs)
write_into_file("/mnt/home/ruthluvu/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/symbotic_medium_50.agents", spawn_locs[:50])

tasks = []

for i in range(10000):
    random_number = random.randint(0, 1)
    if random_number:
        tasks.append(random.choice(inbound_locs))
        tasks.append(random.choice(aisles_locs))
    else:
        tasks.append(random.choice(aisles_locs))
        tasks.append(random.choice(outbound_locs))

write_into_file("/mnt/home/ruthluvu/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/tasks/symbotic_medium.tasks", tasks)

'''map_json_path = "Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/maps/warehouse_small.map"
with open(map_json_path, "r") as f:
    map_json = json.load(f)
    map_json_str = json.dumps(map_json)'''

# sys.path.append('/Users/ruth/WPPL_outer/WPPL/build')

print(sys.path)
# breakpoint()
import py_driver # type: ignore # ignore pylance warning
print(py_driver.playground())

import json
config_path="configs/rhcr_pbs_no_rot.json"
with open(config_path) as f:
    config=json.load(f)
    config_str=json.dumps(config)

agents_paths = [50]

for agents_num in agents_paths:
    ret=py_driver.run(
        # For map, it uses map_path by default. If not provided, it'll use map_json
        # which contains json string of the map
        map_path="Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/maps/symbotic_medium.map",
        # map_json_str = map_json_str,
        # map_json_path = map_json_path,
        simulation_steps=800,
        # for the problem instance we use:
        # if random then we need specify the number of agents and total tasks, also random seed,
        gen_random=False,
        num_agents=agents_num,
        num_tasks=20000,
        seed=0,
        save_paths=True,
        # weight of the left/right workstation, only applicable for maps with workstations
        left_w_weight=1,
        right_w_weight=1,
        # else we need specify agents and tasks path to load data.
        spawn_locs = "Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/symbotic_medium_spawn.agents",
        inbound_locs = "/mnt/home/ruthluvu/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/symbotic_medium_inbound.agents",
        outbound_locs = "/mnt/home/ruthluvu/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/symbotic_medium_outbound.agents",
        aisles_locs = "/mnt/home/ruthluvu/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/symbotic_medium_aisles.agents",
        agents_path=f"Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/symbotic_medium_50.agents",
        tasks_path="Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/tasks/symbotic_medium.tasks",
        # weights are the edge weights, wait_costs are the vertex wait costs
        # if not specified here, then the program will use the one specified in the config file.
        # weights=compressed_weights_json_str,
        # wait_costs=compressed_wait_costs_json_str,    
        # if we don't load config here, the program will load the default config file.
        config=config_str,    
        # the following are some things we don't need to change in the weight optimization case.
        plan_time_limit=1, # in seconds, time limit for planning at each step, no need to change
        preprocess_time_limit=1800, # in seconds, time limit for preprocessing, no need to change
        file_storage_path="large_files/", # where to store the precomputed large files, no need to change
        task_assignment_strategy="greedy", # how to assign tasks to agents, no need to change
        num_tasks_reveal=1, # how many new tasks are revealed, no need to change
    )

    breakpoint()
    #print(ret)