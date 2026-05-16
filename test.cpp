#include <iostream>
#include <vector>
#include <map>


int main ()
{

	std::string ayman = "walid";
	size_t pos = ayman.find("li");
	std::cout << ayman[pos];
	// std::vector<int> numbers;
	// std::map<std::string, int> numbers;
	// numbers.insert(std::make_pair("first", 10));
	// numbers.insert(std::make_pair("second", 11));
	// numbers.insert(std::make_pair("third", 12));
	// numbers.insert(std::make_pair("aurth", 13));
	// numbers.insert(std::make_pair("fifth", 14));
	// std::map<std::string, int>::iterator it = numbers.find("second");
	// if (it != numbers.end())
	// 	std::cout << "the number what we looking for -> " << it->first << " : " << it->second <<  std::endl;
	// else
	// 	std::cout << "second number not found" << std::endl;
	// 	int a = numbers.empty	();
	// 	std::cout << "about the map: " << a << std::endl;
	// 	std::map<std::string, int>::iterator pt;
	// 	for (pt = numbers.begin(); pt != numbers.end(); pt++)
	// 	{
	// 		std::cout << pt->first << " : " << pt->second << std::endl;
	// 	}
	// numbers.push_back(10);
	// numbers.push_back(11);
	// numbers.push_back(12);
	// numbers.push_back(13);

	// std::cout << "first number : " << numbers[0] << std::endl;
	// std::cout << "size of vect : " << numbers.size() << std::endl;

	// for (size_t i = 0; i < numbers.size() ; i++)
	// {
	// 	std::cout << i << " : " << numbers[i] << std::endl;
	// }

	// numbers.pop_back();
	// numbers.pop_back();
	// numbers.pop_back();
	// numbers.pop_back();


	// std::cout << "first number : " << numbers[0] << std::endl;
	// std::cout << "size of vect : " << numbers.size() << std::endl;

	// for (size_t i = 0; i < numbers.size(); i++)
	// {
	// 	std::cout << i << " : " << numbers[i] << std::endl;
	// }
}
