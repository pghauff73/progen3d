#include "editor/application/ApplicationLaunchOptions.h"
#include "editor/application/Progen3dEditorApplication.h"

#include <utility>

int main(int argument_count, char **argument_values)
{
	ApplicationLaunchOptions launch_options =
		ApplicationLaunchOptions::parse(argument_count, argument_values);
	Progen3dEditorApplication application(std::move(launch_options));
	return application.run();
}
