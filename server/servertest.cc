#include <pEp/locked_queue.hh>
#include <thread>
#include <atomic>
#include <vector>
#include <iostream>
#include <set>

unsigned Producers = 7;
unsigned Consumers = 5;
unsigned Elements = 1000*1000;

std::atomic<bool> finished(false);

void do_nothing(uint64_t) {}
utility::locked_queue<uint64_t, &do_nothing>  Q;

std::vector< std::vector<uint64_t> > v;


void produ(unsigned nr)
{
	std::cout << "Producer #" << nr << " starts." << std::endl;
	for(unsigned e = 0; e<Elements; ++e)
	{
		Q.push_back(nr * Elements + e);
	}
	std::cout << "Producer #" << nr << " ends." << std::endl;
}

void consu(unsigned nr)
{
	std::cout << "Consumer #" << nr << " starts." << std::endl;
	while(! (finished && Q.empty()) )
	{
		uint64_t u = ~0ull;
		if( !Q.try_pop_front(u, std::chrono::steady_clock::now() + std::chrono::seconds(2)) )
		{
			std::cout << "Consumer #" << nr << " times out. Got " << v[nr].size() << " elements." << std::endl;
			return;
		}
		v[nr].push_back( u );
	}
	std::cout << "Consumer #" << nr << " ends. Got " << v[nr].size() << " elements." << std::endl;
}

int main(int argc, char** argv)
{
	if(argc>1)
	{
		Producers = atol(argv[1]);
		if(Producers<1 || Producers > 99999)
			Producers = 1;
	}

	if(argc>2)
	{
		Consumers = atol(argv[2]);
		if(Consumers<1 || Consumers > 99999)
			Consumers = 1;
	}

	if(argc>3)
	{
		Elements = atol(argv[3]);
		if(Elements<1 || Elements > 1024*1024*1024)
			Elements = 10000;
	}
	
	std::cout << "Testing with " << Producers << " producer threads, " << Consumers << " consumer threads, " << Elements << " elements per thread." << std::endl;
	
	v.resize(Consumers);
	std::vector<std::thread> vp; vp.reserve(Producers);
	std::vector<std::thread> vc; vc.reserve(Consumers);
	
	for(unsigned c = 0; c<Consumers; ++c)
	{
		std::thread th(consu, c);
		vc.push_back( std::move(th) );
	}

	for(unsigned p = 0; p<Producers; ++p)
	{
		std::thread th( produ, p);
		vp.emplace_back( std::move(th) );
	}
	
	std::cout << "Waiting for all producers to finish..." << std::endl;
	for(auto& t : vp)
	{
		t.join();
	}
	
	finished=true;
	std::cout << "Waiting for all consumers to finish..." << std::endl;
	for(auto& t : vc)
	{
		t.join();
	}
	
	std::cout << "All threads done. Now collecting data" << std::endl;
	
	uint64_t total_elements = 0;
	std::set<uint64_t> s;
	for(unsigned c=0; c<Consumers; ++c)
	{
		total_elements += v[c].size();
		s.insert( v[c].begin(), v[c].end() );
	}
	
	std::cout << "Expecting " << (Elements*Producers) << " elements. Got " << total_elements << " elements, " << s.size() << " distinct elements.\n";
	if( ((Elements*Producers) == s.size()) && (s.size() == total_elements) )
	{
		std::cout << "\tOK. :-)\n";
		return 0;
	}else{
		std::cout << "\tERROR!\n";
		return 1;
	}
}
