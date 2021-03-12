#include "context.hh"

void Context::store(int position, size_t value)
{
	obj_store.emplace( position, value );
}

size_t Context::retrieve(int position)
{
	return obj_store.at(position);
}


void Context::clear()
{
	obj_store.clear();
}
