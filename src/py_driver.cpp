#include "CompetitionSystem.h"
#include "Evaluation.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include "nlohmann/json.hpp"
#include <signal.h>
#include <ctime>
#include <climits>
#include <memory>
#include <util/Analyzer.h>
#include "nlohmann/json.hpp"

#ifdef MAP_OPT
#include "util/analyze.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#endif

namespace po = boost::program_options;
using json = nlohmann::json;

namespace py = pybind11;

// po::variables_map vm;
std::unique_ptr<BaseSystem> system_ptr;


// void sigint_handler(int a)
// {
//     fprintf(stdout, "stop the simulation...\n");
//     if (!vm["evaluationMode"].as<bool>())
//     {
//         system_ptr->saveResults(vm["output"].as<std::string>());
//     }
//     _exit(0);
// }


int _get_Manhattan_distance(int loc1, int loc2, int cols) {
    return abs(loc1 / cols - loc2 / cols) + abs(loc1 % cols - loc2 % cols);
}

std::shared_ptr<std::vector<float> > weight_format_conversion(Grid & grid, std::vector<float> & weights)
{
    const int max_weight=100000;
    std::shared_ptr<std::vector<float> > map_weights_ptr = std::make_shared<std::vector<float> >(grid.map.size()*5, max_weight);
    auto & map_weights=*map_weights_ptr;

    const int dirs[4]={1,-grid.cols, -1, grid.cols};
    const int map_weights_idxs[4]={0,3,2,1};

    int j=0;

    ++j; // the 0 indexed weight is for wait

    for (int i=0;i<grid.map.size();++i) {
        if (grid.map[i] == 1) {
            continue;
        }

        map_weights[i*5+4] = weights[0];

        for (int d=0;d<4;++d) {
            int dir=dirs[d];
            if (
                0<=i+dir && i+dir<grid.map.size() &&
                _get_Manhattan_distance(i, i+dir, grid.cols) <= 1 &&
                grid.map[i+dir] != 1
            ) {
                float weight = weights.at(j);
                if (weight==-1) {
                    weight=max_weight;
                }

                int map_weight_idx=map_weights_idxs[d];
                map_weights[i*5+map_weight_idx]=weight;
                ++j;
            }
        }
    }

    // std::cout<<"map weights: ";
    // for (auto i=0;i<map_weights.size();++i) {
    //     std::cout<<map_weights[i]<<" ";
    // }
    // std::cout<<endl;

    if (j!=weights.size()) {
        std::cout<<"weight size mismatch: "<<j<<" vs "<<weights.size()<<std::endl;
        exit(1);
    }

    return map_weights_ptr;
}


std::shared_ptr<std::vector<float> > weight_format_conversion_with_wait_costs(Grid & grid, std::vector<float> & edge_weights, std::vector<float> & wait_costs)
{
    const int max_weight=100000;
    std::shared_ptr<std::vector<float> > map_weights_ptr = std::make_shared<std::vector<float> >(grid.map.size()*5, max_weight);
    auto & map_weights=*map_weights_ptr;

    const int dirs[4]={1,-grid.cols, -1, grid.cols};
    const int map_weights_idxs[4]={0,3,2,1};

    // read wait cost
    int j=0;
    for (int i=0;i<grid.map.size();++i) {
        if (grid.map[i] == 1) {
            continue;
        }
        map_weights[i*5+4] = wait_costs[j];
        ++j;
    }

    if (j!=wait_costs.size()) {
        std::cout<<"wait cost size mismatch: "<<j<<" vs "<<wait_costs.size()<<std::endl;
        exit(1);
    }


    // read edge cost
    j=0;
    for (int i=0;i<grid.map.size();++i) {
        if (grid.map[i] == 1) {
            continue;
        }

        for (int d=0;d<4;++d) {
            int dir=dirs[d];
            if (
                0<=i+dir && i+dir<grid.map.size() &&
                _get_Manhattan_distance(i, i+dir, grid.cols) <= 1 &&
                grid.map[i+dir] != 1
            ) {
                float weight = edge_weights.at(j);
                if (weight==-1) {
                    weight=max_weight;
                }

                int map_weight_idx=map_weights_idxs[d];
                map_weights[i*5+map_weight_idx]=weight;
                ++j;
            }
        }
    }

    // std::cout<<"map weights: ";
    // for (auto i=0;i<map_weights.size();++i) {
    //     std::cout<<map_weights[i]<<" ";
    // }
    // std::cout<<endl;

    if (j!=edge_weights.size()) {
        std::cout<<"edge weight size mismatch: "<<j<<" vs "<<edge_weights.size()<<std::endl;
        exit(1);
    }

    return map_weights_ptr;
}


void gen_random_instance(Grid & grid, std::vector<int> & agents, std::vector<int> & tasks, int num_agents, int num_tasks, uint seed) {
    std::mt19937 MT(seed);

    std::vector<int> empty_locs;
    for (int i=0;i<grid.map.size();++i) {
        if (grid.map[i] == 0) {
            empty_locs.push_back(i);
        }
    }

    std::shuffle(empty_locs.begin(), empty_locs.end(), MT);

    for (int i=0;i<num_agents;++i) {
        agents.push_back(empty_locs[i]);
    }

    if (grid.end_points.size()>0) {
        // only sample goal locations from end_points
        std::cout<<"sample goal locations from end points"<<std::endl;
        for (int i=0;i<num_tasks;++i) {
            int rnd_idx=MT()%grid.end_points.size();
            tasks.push_back(grid.end_points[rnd_idx]);
        }        
    } else {
        std::cout<<"sample goal locations from empty locations"<<std::endl;
        for (int i=0;i<num_tasks;++i) {
            int rnd_idx=MT()%empty_locs.size();
            tasks.push_back(empty_locs[rnd_idx]);
        }
    }
}

std::string run(const py::kwargs& kwargs)
{    
    int simulation_steps=kwargs["simulation_steps"].cast<int>();
    double plan_time_limit=kwargs["plan_time_limit"].cast<double>();
    double preprocess_time_limit=kwargs["preprocess_time_limit"].cast<double>();
    // std::string map_path=kwargs["map_path"].cast<std::string>();
    std::string file_storage_path=kwargs["file_storage_path"].cast<std::string>();
    std::string task_assignment_strategy=kwargs["task_assignment_strategy"].cast<std::string>();
    int num_tasks_reveal=kwargs["num_tasks_reveal"].cast<int>();

    // Read in left and right weights
    double left_w_weight = 1;
    double right_w_weight = 1;
    if (kwargs.contains("left_w_weight"))
    {
        left_w_weight = kwargs["left_w_weight"].cast<double>();
    }
    if (kwargs.contains("right_w_weight"))
    {
        right_w_weight = kwargs["right_w_weight"].cast<double>();
    }

    // Read in map. Use map_path by default. If not provided, use map_json
    Grid grid;
    if (kwargs.contains("map_path"))
    {
        std::string map_path = kwargs["map_path"].cast<std::string>();
        grid.load_map_from_path(map_path, left_w_weight, right_w_weight);
    }
    else if (kwargs.contains("map_json_str") ||
             kwargs.contains("map_json_path"))
    {
        json map_json;
        if (kwargs.contains("map_json_str"))
        {
            std::string map_json_str = kwargs["map_json_str"].cast<std::string>();
            map_json = json::parse(map_json_str);
        }
        else
        {
            std::string map_json_path = kwargs["map_json_path"].cast<std::string>();
            std::ifstream f(map_json_path);
            map_json = json::parse(f);
        }
        grid.load_map_from_json(map_json, left_w_weight, right_w_weight);
    }

    // // should be a command line string running the code
    // std::string cmd=kwargs["cmd"].cast<std::string>();
    // std::cout<<"cmd from python is: "<<cmd<<std::endl;

    // // Declare the supported options.
    // po::options_description desc("Allowed options");
    // desc.add_options()("help", "produce help message")
    //     // ("inputFolder", po::value<std::string>()->default_value("."), "input folder")
    //     ("inputFile,i", po::value<std::string>()->required(), "input file name")
    //     ("output,o", po::value<std::string>()->default_value("./test.json"), "output file name")
    //     ("evaluationMode", po::value<bool>()->default_value(false), "evaluate an existing output file")
    //     ("simulationTime", po::value<int>()->default_value(5000), "run simulation")
    //     ("fileStoragePath", po::value<std::string>()->default_value(""), "the path to the storage path")
    //     ("planTimeLimit", po::value<int>()->default_value(INT_MAX), "the time limit for planner in seconds")
    //     ("preprocessTimeLimit", po::value<int>()->default_value(INT_MAX), "the time limit for preprocessing in seconds")
    //     ("logFile,l", po::value<std::string>(), "issue log file name");
    // clock_t start_time = clock();
    // // po::store(po::parse_command_line(argc, argv, desc), vm);

    // po::store(po::command_line_parser(po::split_unix(cmd)).options(desc).run(), vm);

    // if (vm.count("help"))
    // {
    //     std::cout << desc << std::endl;
    //     exit(-1);
    // }

    // po::notify(vm);

    // std::string base_folder = vm["inputFolder"].as<std::string>();
    // boost::filesystem::path p(vm["inputFile"].as<std::string>());

    // ONLYDEV(
    //     const auto & filename=p.filename();
    //     analyzer.data["instance"]=filename.c_str();
    // )

    // boost::filesystem::path dir = p.parent_path();
    // std::string base_folder = dir.string();
    // std::cout << base_folder << std::endl;
    // if (base_folder.size() > 0 && base_folder.back() != '/')
    // {
    //     base_folder += "/";
    // }

    Logger *logger = new Logger();
    // if (vm.count("logFile"))
    //     logger->set_logfile(vm["logFile"].as<std::string>());

    MAPFPlanner *planner = nullptr;
    // Planner is inited here, but will be managed and deleted by system_ptr deconstructor
    // if (vm["evaluationMode"].as<bool>())
    // {
    //     logger->log_info("running the evaluation mode");
    //     planner = new DummyPlanner(vm["output"].as<std::string>());
    // }
    // else
    // {
        planner = new MAPFPlanner();
    // }

    // auto input_json_file = vm["inputFile"].as<std::string>();
    // json data;
    // std::ifstream f(input_json_file);
    // try
    // {
    //     data = json::parse(f);
    // }
    // catch (json::parse_error error)
    // {
    //     std::cerr << "Failed to load " << input_json_file << std::endl;
    //     std::cerr << "Message: " << error.what() << std::endl;
    //     exit(1);
    // }

    // auto map_path = read_param_json<std::string>(data, "mapFile");
    // Grid grid(base_folder + map_path);
    // Grid grid(map_path);

    // planner->env->map_name = map_path.substr(map_path.find_last_of("/") + 1);
    planner->env->map_name = grid.map_name;
    // planner->env->file_storage_path = vm["fileStoragePath"].as<std::string>();
    planner->env->file_storage_path = file_storage_path;

    if (kwargs.contains("config")){
        std::string config_str=kwargs["config"].cast<std::string>();
        nlohmann::json config=nlohmann::json::parse(config_str);
        planner->config=config;
    }

    if (kwargs.contains("weights")) {
        std::string weight_str=kwargs["weights"].cast<std::string>();
        nlohmann::json weight_json=nlohmann::json::parse(weight_str);
        std::vector<float> weights;
        for (auto & w:weight_json) {
            weights.push_back(w.get<float>());
        }

        if (kwargs.contains("wait_costs")) {
            std::string wait_costs_str=kwargs["wait_costs"].cast<std::string>();
            nlohmann::json wait_costs_json=nlohmann::json::parse(wait_costs_str);
            std::vector<float> wait_costs;
            for (auto & w:wait_costs_json) {
                wait_costs.push_back(w.get<float>());
            }


            planner->map_weights=weight_format_conversion_with_wait_costs(grid, weights, wait_costs);
        } else {
            planner->map_weights=weight_format_conversion(grid, weights);
        }

    } 

    ActionModelWithRotate *model = new ActionModelWithRotate(grid);
    model->set_logger(logger);

    if (grid.agent_home_locations.size()==0 || grid.end_points.size()==0) {
        std::vector<int> agents;
        std::vector<int> tasks;

        std::vector<int> inbounds;
        std::vector<int> outbounds;
        std::vector<int> aisles;

        std::vector<int> loading_track;

        if (!kwargs.contains("gen_random") || !kwargs["gen_random"].cast<bool>()){
            if (!kwargs.contains("agents_path") || !kwargs.contains("tasks_path")){
                logger->log_fatal("agents_path and tasks_path must be provided if not generate instance randomly");
                exit(1);
            }
            auto agents_path = kwargs["agents_path"].cast<std::string>(); //useless
            auto tasks_path = kwargs["tasks_path"].cast<std::string>();
            int seed = kwargs["seed"].cast<int>();
            agents = read_int_vec(agents_path); //useless
            tasks = read_int_vec(tasks_path);

            cout << "SEED " << seed << "\n";

            std::srand(seed);
            // cout << rand() << "\n";

            auto spawn_path = kwargs["spawn_locs"].cast<std::string>();
            std::vector<int> spawns = read_int_vec(spawn_path);
            shuffle(spawns.begin(), spawns.end(), std::default_random_engine());

            for(int i = 0; i < spawns.size(); i++){
                std::cout << spawns[i] << ", ";
            }
            std::cout << "\n";

            std::vector<bool> used(spawns.size(), false);
            
            // aisles, inbounds, and outbounds
            auto aisles_path = kwargs["aisles_locs"].cast<std::string>();
            auto inbound_path = kwargs["inbound_locs"].cast<std::string>();
            auto outbound_path = kwargs["outbound_locs"].cast<std::string>();

            inbounds = read_int_vec(inbound_path);
            outbounds = read_int_vec(outbound_path);
            aisles = read_int_vec(aisles_path);

            for (int k = 0; k < agents.size();)
            {
                int idx = rand() % spawns.size();
                int loc = spawns[idx];
                // if (G.types[loc] = "Home" && !used[loc])
                if (!used[idx])
                {
                    agents[k] = spawns[idx];
                    // paths[k].emplace_back(starts[k]);
                    used[idx] = true;
                    // finished_tasks[k].emplace_back(loc, 0);

                    std::cout << k << " " << spawns[idx] << "\n";
                    k++;
                    // come back here
                }
            }

            for (int k = 0; k < agents.size(); k++)
            {
                int goal;
                goal = aisles[rand()%aisles.size()];
                tasks[k] = goal;
                if (rand() % 2 == 0)
                {
                    loading_track.push_back(0); // loading after the goal is arrived, 0 is without loading, 1 is with loading
                }
                else
                {
                    loading_track.push_back(1);
                }
            }

        } else {
            int num_agents=kwargs["num_agents"].cast<int>();
            int num_tasks=kwargs["num_tasks"].cast<int>();
            uint seed=kwargs["seed"].cast<uint>();
            gen_random_instance(grid, agents, tasks, num_agents, num_tasks, seed);
        }

        std::cout << agents.size() << " agents and " << tasks.size() << " tasks"<< std::endl;
        if (agents.size() > tasks.size())
            logger->log_warning("Not enough tasks for robots (number of tasks < team size)");

        // std::string task_assignment_strategy = data["taskAssignmentStrategy"].get<std::string>();
        if (task_assignment_strategy == "greedy")
        {
            system_ptr = std::make_unique<TaskAssignSystem>(grid, planner, agents, tasks, inbounds, outbounds, aisles, loading_track, model);
        }
        else if (task_assignment_strategy == "roundrobin")
        {
            system_ptr = std::make_unique<InfAssignSystem>(grid, planner, agents, tasks, model);
        }
        else if (task_assignment_strategy == "roundrobin_fixed")
        {
            std::vector<vector<int>> assigned_tasks(agents.size());
            for (int i = 0; i < tasks.size(); i++)
            {
                assigned_tasks[i % agents.size()].push_back(tasks[i]);
            }
            system_ptr = std::make_unique<FixedAssignSystem>(grid, planner, agents, assigned_tasks, model);
        }
        else
        {
            std::cerr << "unkown task assignment strategy " << task_assignment_strategy << std::endl;
            logger->log_fatal("unkown task assignment strategy " + task_assignment_strategy);
            exit(1);
        }
    } else {
        if (!kwargs.contains("gen_random") || !kwargs["gen_random"].cast<bool>()){
            logger->log_fatal("must generate instance randomly");
            exit(1);
        }

        int num_agents=kwargs["num_agents"].cast<int>();
        std::cout << "using kiva system (random task generation) with "<<num_agents<<" agents"<<std::endl;
        
        uint seed=kwargs["seed"].cast<uint>();
        system_ptr = std::make_unique<KivaSystem>(grid,planner,model,num_agents,seed);
        // system_ptr = std::make_unique<FixedAssignSystem>(grid, planner, agents, assigned_tasks, model);
    }

    system_ptr->set_logger(logger);
    system_ptr->set_plan_time_limit(plan_time_limit);
    system_ptr->set_preprocess_time_limit(preprocess_time_limit);

    system_ptr->set_num_tasks_reveal(num_tasks_reveal);

    // signal(SIGINT, sigint_handler);

    clock_t start_time = clock();
    system_ptr->simulate(simulation_steps);
    double runtime = (double)(clock() - start_time)/ CLOCKS_PER_SEC;

    nlohmann::json analysis=system_ptr->analyzeResults();
    analysis["cpu_runtime"] = runtime;

    // Save path if applicable
    if (kwargs.contains("save_paths") && kwargs["save_paths"].cast<bool>())
    {
        boost::filesystem::path output_dir(kwargs["file_storage_path"].cast<std::string>());
        boost::filesystem::create_directories(output_dir);
        boost::filesystem::path path_file = output_dir / "results.json";
        system_ptr->saveResults(path_file.string());
    }

    // system_ptr->saveResults("debug.json");
    return analysis.dump(4);

    // if (!vm["evaluationMode"].as<bool>())
    // {
    //     system_ptr->saveResults(vm["output"].as<std::string>());
    // }

    // delete model;
    // delete logger;
    // return 0;
}

string playground(){
	std::string json_string = R"(
	{
		"pi": 3.141,
		"happy": true
	}
	)";
	json ex1 = json::parse(json_string);

	cout << ex1["pi"] << endl;

	return ex1.dump();
}


PYBIND11_MODULE(py_driver, m) {
	// optional module docstring
    // m.doc() = ;

    m.def("playground", &playground, "Playground function to test everything");
    // m.def("add", &add, py::arg("i")=0, py::arg("j")=1);
    m.def("run", &run, "Function to run warehouse simulation");
}