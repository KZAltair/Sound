#include <iostream>
#include "Sound.h"

int main()
{
	Sound s(L"test.wav");

	char userInput = ' ';

	while (userInput != 'Q' || userInput != 'q')
	{
		std::cout << "Play music (P) Quit (Q)" << std::endl;
		std::cout << "Enter the command: ";
		std::cin >> userInput;

		if (userInput == 'P' || userInput == 'p')
		{
			s.Play();
			continue;
		}
		if (userInput != 'Q' || userInput != 'q')
		{
			break;
		}

	}

	system("pause");
	return 0;
}