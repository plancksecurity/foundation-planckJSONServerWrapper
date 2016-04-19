// A "registry" for arbitrary objects.
// all registered objects have their unique "handle", an arbitrary
// 64 bit number that can also be represented as a base64-encoded string

#include <cstdint>
#include <string>
#include <map>
#include <memory>  // for unique ptr
#include <vector>

//////////////////////////////
//
// helper functions
//
/////////////////////////////

// mix the bits of 'value' into a less-
std::uint64_t joggle(std::uint64_t realm, std::uint64_t value);

// unjoggle( joggle(x) ) == x for any 64 bit value
std::uint64_t unjoggle(std::uint64_t realm, std::uint64_t value);

// converts 'x' into a 11-character string
std::string     base57(std::uint64_t x);

// does the reverse of base62().
std::uint64_t unbase57(const std::string& s);


// tests the joggle(), unjoggle(), base57(), unbase57() on several values.
// return 0 on succes, >0 on error
unsigned test_joggle();


template<class Resource, class Creator, class Deleter>
class Registry
{
public:
	Registry(const Creator& cr, const Deleter& del)
	: c(cr), d(del)
	{}
	 
	Registry(const Registry&) = delete;
	
	~Registry()
	{
		clear();
	}
	
	bool has( std::uint64_t index ) const;
	bool has( const std::string& handle ) const;

	// throws an exception if there is no such element in the map
	Resource get( std::uint64_t handle ) const
	{
		const auto it = m.find(handle);
		if(it == m.end())
		{
			throw std::runtime_error("There is no element with handle <" + std::to_string(handle) + "> registered.");
		}
		return it->second;
	}
	
	Resource get( const std::string& handle ) const
	{
		return get(unbase57(handle));
	}

	// construct & inserts a new element and returns the handle
	template<class... Args>
	std::uint64_t emplace(Args... args)
	{
		Resource rsrc = c(args...);
		const uint64_t id = joggle( Identifier, intptr_t(rsrc) );
		const auto ret = m.insert( std::make_pair(id, rsrc) );
		if(ret.second == false)
		{
			d(rsrc);
			throw std::runtime_error("Duplicate entry");
		}
		return id;
	}

	void erase( std::uint64_t handle )
	{
		auto it = m.find(handle);
		if(it!=m.end())
		{
			d(it->second);
			m.erase(it);
		}
	}

	void erase( const std::string& handle )
	{
		erase(unbase57(handle));
	}

	std::vector<std::uint64_t> keys() const
	{
		std::vector<std::uint64_t> k;
		k.reserve(m.size());
		for(const auto& e : m)
		{
			k.push_back(e.first);
		}
		return k;
	}

	void clear()
	{
		for(auto elem : m)
		{
			d(elem.second);
		}
	}
	
	std::size_t size() const
	{
		return m.size();
	}

private:
	typedef std::map< std::uint64_t, Resource > Map;
	
	const Creator c;
	const Deleter d;
	Map m;
	
	static const uint64_t Identifier;
};
