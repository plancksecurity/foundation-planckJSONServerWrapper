#include "context.hh"

void Context::store(int position, json_spirit::Object& obj)
{
	obj_store.emplace( position, std::ref(obj) );
}

json_spirit::Object& Context::retrieve(int position)
{
	return obj_store.at(position);
}


void Context::clear()
{
	obj_store.clear();
}
