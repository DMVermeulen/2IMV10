#include"Application.h"

// Main code
int main(int, char**)
{
	Scene scene = Scene();
	Application app(scene);
	app.run();
	return 0;
}