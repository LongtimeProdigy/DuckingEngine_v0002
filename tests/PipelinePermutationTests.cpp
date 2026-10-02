#include "../PipelinePermutation.h"
#include <cassert>
#include <iostream>

int main()
{
	std::vector<std::vector<std::string>> combinations;
	std::string error;
	auto parse = [&](const std::string& children)
	{
		TiXmlDocument document;
		const std::string xml = "<Pipeline>" + children + "</Pipeline>";
		document.Parse(xml.c_str());
		assert(!document.Error());
		return DK::parsePipelinePermutations(*document.RootElement(), combinations, error);
	};
	assert(parse(""));
	assert(combinations.size() == 1 && combinations[0].empty());
	assert(parse("<Permutation Name='USE_FOG' Values='0, 1'/><Permutation Name='QUALITY' Values='1,2,3'/>"));
	assert(combinations.size() == 6);
	for (size_t i = 0; i < 6; ++i)
	{
		assert(combinations[i][0] == "USE_FOG=" + std::to_string(i / 3));
		assert(combinations[i][1] == "QUALITY=" + std::to_string(i % 3 + 1));
	}
	assert(DK::pipelinePermutationName("Example", 0) == "Example");
	assert(DK::pipelinePermutationName("Example", 5) == "Example[5]");
	const char* invalid[] = {
		"<Permutation Values='0,1'/>", "<Permutation Name='A'/>",
		"<Permutation Name='' Values='0'/>", "<Permutation Name='1A' Values='0'/>",
		"<Permutation Name='A-B' Values='0'/>", "<Permutation Name='A' Values=''/>",
		"<Permutation Name='A' Values='0,'/>", "<Permutation Name='A' Values=',0'/>",
		"<Permutation Name='A' Values='0, ,1'/>", "<Permutation Name='A' Values='0, 0'/>",
		"<Permutation Name='A' Values='0'/><Permutation Name='A' Values='1'/>"
	};
	for (const auto* input : invalid)
	{
		assert(!parse(input));
		assert(!error.empty());
	}
	std::string dimensions;
	for (int i = 0; i < 12; ++i)
		dimensions += "<Permutation Name='P" + std::to_string(i) + "' Values='0,1'/>";
	assert(parse(dimensions));
	assert(combinations.size() == 4096);
	assert(!parse(dimensions + "<Permutation Name='OVERFLOW' Values='0,1'/>"));
	assert(parse("<Permutation Name='_VALID_2' Values=' (1 &lt;&lt; 2) '/>"));
	assert(combinations[0][0] == "_VALID_2=(1 << 2)");
	std::cout << "Pipeline permutation tests passed.\n";
}
