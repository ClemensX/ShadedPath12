#pragma once

enum RenderPlanStepType { PlanRender, PlanSync, PlanFinish};
class RenderPlanStep {
public:
	RenderPlanStepType type;
	int numCommands;
};

class RenderPlan {
public:
	// add render phase to plan with given number of commands
	RenderPlan* addRender(int numCommands);
	// wait until all previous commands are finished
	RenderPlan* addSync();
	void finish();
	string describe();
	RenderPlanStep &stepRef(int i);
private:
	vector<RenderPlanStep> steps;
};