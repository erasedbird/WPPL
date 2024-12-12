#pragma once
#include "common.h"
#include "ActionModel.h"
#include "SharedEnv.h"
#include "RHCR/main/MAPFSolver.h"
#include "RHCR/interface/CompetitionGraph.h"
#include "RHCR/main/BasicSystem.h"
#include <thread>
#include <future>
#include <mutex>
#include "util/CompetitionActionModel.h"

namespace RHCR {
class RHCRSolver: public BasicSystem{
// for simplicity, just make everything public. but attributes or functions start with _ are supposed to use privately in general cases.
public:

    //***** attributes *****//
    // std::thread task;
    // std::future<void()> future;
    // this deque maintains actions planned into future. we can directly retrieve the actions for the current step from this deque.
    // TODO it should be protected by a mutex
    // bool stop_flag;

    CompetitionGraph& graph;
    CompetitionActionModelWithRotate model;
    bool need_replan = true;
    int total_feasible_timestep = 0;

    //***** functions *****//
    RHCRSolver(CompetitionGraph & graph, MAPFSolver & mapf_solver, SharedEnvironment * env): BasicSystem(graph,mapf_solver), graph(graph), model(env) {};

    void start_plan_task();
    void stop_plan_task();

    // this function plans actions into future (possible from a certain state?)
    void plan(const SharedEnvironment & env);

    void get_step_actions(const SharedEnvironment & env, vector<Action> & actions);

    // inline void set_mapf_solver(const shared_ptr<MAPFSolver> & mapf_solver){
    //     this->mapf_solver=mapf_solver;
    // };

    // inline void set_map(const shared_ptr<BasicGraph> & map){
    //     this->map=map;
    // };

    void initialize(const SharedEnvironment & env);
    void update_goal_locations(const SharedEnvironment & env);
    void set_parameters(const string & map_name);

    Action get_action_from_states(const State & state, const State & next_state, bool consider_rotation){
#ifndef NO_ROT
        assert(state.timestep+1==next_state.timestep);
        
        if (state.location==next_state.location){
            // either turn or wait
            if (state.orientation==next_state.orientation) {
                return Action::W;
            } else if ((state.orientation-next_state.orientation+4)%4==3) {
                return Action::CR;
            } else if ((state.orientation-next_state.orientation+4)%4==1) {
                return Action::CCR;
            } else {
                assert(false);
                return Action::W;
            }
            }
        else {
            return Action::FW;
        }
#else 
        assert(state.timestep+1==next_state.timestep);
        
        for (int i=0;i<5;++i) {
            if (state.location+model.moves[i]==next_state.location) {
                return static_cast<Action>(i);
            }
        }

        cerr<<"Cannot get action from invalid movement between state"<<state<<" and "<<next_state<<endl;
        exit(-1);
#endif
    }
};
}