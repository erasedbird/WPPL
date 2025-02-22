import sys
import os
# sys.path.append(os.path.abspath("build/python"))
sys.path.append(os.path.abspath("build"))
sys.path.append('scripts')
from map import Map
import json

map_path="example_problems/warehouse.domain/maps/kiva_large_w_mode.map"
# full_weight_path="scripts/random_weight_001.w"
with_wait_costs=True

map=Map(map_path)
map.print_graph(map.graph)

map_json_path = "../maps/warehouse/human/kiva_large_w_mode_no_end.json"
with open(map_json_path, "r") as f:
    map_json = json.load(f)
    map_json_str = json.dumps(map_json)

# sys.path.append('/Users/ruth/WPPL_outer/WPPL/build')

print(sys.path)
# breakpoint()
import py_driver # type: ignore # ignore pylance warning
print(py_driver.playground())

import json
config_path="configs/pibt_default_no_rot.json"
with open(config_path) as f:
    config=json.load(f)
    config_str=json.dumps(config)

agents_paths = [10, 50, 60, 100, 200, 400, 800]

for agents_num in agents_paths:
    ret=py_driver.run(
        # For map, it uses map_path by default. If not provided, it'll use map_json
        # which contains json string of the map
        map_path="/Users/ruth/WPPL_outer/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/maps/warehouse_small.map",
        # map_json_str = map_json_str,
        # map_json_path = map_json_path,
        simulation_steps=500,
        # for the problem instance we use:
        # if random then we need specify the number of agents and total tasks, also random seed,
        gen_random=False,
        num_agents=agents_num,
        num_tasks=100000,
        seed=0,
        save_paths=True,
        # weight of the left/right workstation, only applicable for maps with workstations
        left_w_weight=1,
        right_w_weight=1,
        # else we need specify agents and tasks path to load data.
        agents_path=f"/Users/ruth/WPPL_outer/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/agents/warehouse_small_{agents_num}.agents",
        tasks_path="/Users/ruth/WPPL_outer/WPPL/Benchmark-Archive/2023 Competition/Example Instances/warehouse.domain/tasks/warehouse_small.tasks",
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
                
            