#include "pch.h"

RenderPlan* RenderPlan::addRender(int numCommands)
{
	RenderPlanStep step;
	step.type = PlanRender;
	step.numCommands = numCommands;
	steps.push_back(step);
	return this;
}

RenderPlan* RenderPlan::addSync()
{
	RenderPlanStep step;
	step.type = PlanSync;
	step.numCommands = 0;
	steps.push_back(step);
	return this;
}

void RenderPlan::finish()
{
	RenderPlanStep step;
	step.type = PlanFinish;
	step.numCommands = 0;
	steps.push_back(step);
}

string RenderPlan::describe()
{
	stringstream s;
	//s << "" << endl;
	for each (auto& step in steps)
	{
		switch (step.type) {
		case PlanRender: s << "Render"; break;
		case PlanSync: s << "Sync"; break;
		case PlanFinish: s << "Finish"; break;
		}
		if (step.type == PlanRender) s << "(" << step.numCommands << ")";
		s << " ";
	}
	return s.str();
}

RenderPlanStep & RenderPlan::stepRef(int i)
{
	return steps[i];
}
