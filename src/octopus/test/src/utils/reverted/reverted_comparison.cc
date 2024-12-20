#include "reverted_comparison.hh"


namespace std
{
	std::ostream &operator<<(std::ostream &oss, StreamedEntityRecord const &rec)
	{
		oss << "SER[";
		std::for_each(rec.records.begin(), rec.records.end(), [&oss](std::string const &entry)
		{
			oss << entry <<std::endl;
		});
		oss <<"]";
		return oss;
	}
}