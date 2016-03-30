#include "pep-types.hh"
#include "json_spirit/json_spirit_utils.h"

namespace
{
	// fetch a member from the given object, if set. (return a sensible NULL/default value if not)
	template<class T, js::Value_type VALUE_TYPE>
	T from_json_object(const js::Object& obj, const std::string& key)
	{
		const auto v = find_value(obj, key);
		if(v.type() == js::null_type) return nullptr;
		if(v.type() == VALUE_TYPE) return from_json<T>(v);
		
		throw std::runtime_error("JSON object has a member for key \"" + key + "\""
			" with incompatible type " + js::value_type_to_string( v.type())
			+ " instead of expected type " + js::value_type_to_string(VALUE_TYPE)
			);
	}
	
	template<class T>
	void to_json_object(js::Object& obj, const std::string& key, const T& value)
	{
		if(value!=nullptr)
		{
			obj.emplace_back( key, js::Value( to_json<T>(value) ));
		}
	}

}


template<>
In<message*>::~In()
{
	free_message(value);
}

template<>
In<stringlist_t*>::~In()
{
	free_stringlist(value);
}


template<>
In<pEp_identity*>::~In()
{
	free_identity(value);
}


template<>
In<PEP_enc_format>::~In()
{
	// nothing to do here. :-)
}


template<>
Out<stringlist_t*>::Out(const Out<stringlist_t*>& other)
: value( new stringlist_t* )
{
	*value = *other.value ? stringlist_dup(*other.value) : nullptr;
}

template<>
Out<stringlist_t*>::~Out()
{
	if(value) free_stringlist(*value);
	delete value;
}


template<>
Out<_message*>::Out(const Out<_message*>& other)
: value( new _message* )
{
	*value = *other.value ? message_dup(*other.value) : nullptr;
}


template<>
Out<_message*>::~Out()
{
	if(value) free_message(*value);
	delete value;
}


template<>
Out<PEP_color>::Out(const Out<PEP_color>& other)
: value( new PEP_color(*other.value) )
{
}

template<>
Out<PEP_color>::~Out()
{
	delete value;
}


template<>
message* from_json<message*>(const js::Value& v)
{
	const js::Object& o = v.get_obj();

	_message* msg = new_message(PEP_dir_incoming);

	// fetch values from v and put them into msg
	msg->id       = from_json_object<char*, js::str_type>(o, "id");
	msg->shortmsg = from_json_object<char*, js::str_type>(o, "short");
	msg->longmsg  = from_json_object<char*, js::str_type>(o, "long");
	msg->longmsg_formatted = from_json_object<char*, js::str_type>(o, "fmt");
	msg->from     = from_json_object<pEp_identity*, js::obj_type>(o, "from");
	msg->to       = from_json_object<identity_list*, js::array_type>(o, "to");
	msg->recv_by  = from_json_object<pEp_identity*, js::obj_type>(o, "recv_by");
	msg->cc       = from_json_object<identity_list*, js::array_type>(o, "cc");
	msg->bcc      = from_json_object<identity_list*, js::array_type>(o, "bcc");
	msg->reply_to = from_json_object<identity_list*, js::array_type>(o, "reply_to");
	msg->in_reply_to = from_json_object<stringlist_t*, js::array_type>(o, "in_reply_to");
	// TODO: remaining data members! –.–
	
	return msg;
}


template<>
pEp_identity* from_json<pEp_identity*>(const js::Value& v)
{
	const js::Object& o = v.get_obj();
	
	char* address     = from_json_object<char*, js::str_type>(o, "address");
	char* fingerprint = from_json_object<char*, js::str_type>(o, "fpr");
	char* user_id     = from_json_object<char*, js::str_type>(o, "user_id");
	char* username    = from_json_object<char*, js::str_type>(o, "username");
	
	pEp_identity* ident = new_identity
		(
			address,
			fingerprint,
			user_id,
			username
		);
	
	free(username);
	free(user_id);
	free(fingerprint);
	free(address);
	
	return ident;
}


template<>
js::Value to_json<message*>(message* const& msg)
{
	if(msg == nullptr)
	{
		return js::Value("NULL-MESSAGE");
	}
	
	js::Object o;
	to_json_object(o, "id"   , msg->id);
	to_json_object(o, "shortmsg", msg->shortmsg);
	to_json_object(o, "longmsg" , msg->longmsg);
	to_json_object(o, "longmsg_formatted"  , msg->longmsg_formatted);
	to_json_object(o, "from" , msg->from);
	to_json_object(o, "recv_by" , msg->recv_by);
	
	to_json_object(o, "to"   , msg->to);
	to_json_object(o, "cc"   , msg->cc);
	to_json_object(o, "bcc"   , msg->bcc);
	to_json_object(o, "reply_to"   , msg->reply_to);
	to_json_object(o, "in_reply_to"   , msg->in_reply_to);
	
	
	return js::Value( o );
}


template<>
stringlist_t* from_json<stringlist_t*>(const js::Value& v)
{
	const js::Array& a = v.get_array();

	stringlist_t* sl = new_stringlist( nullptr );

	for(const js::Value& v : a )
	{
		const std::string s = v.get_str();
		stringlist_add(sl, s.c_str() );
	}

	return sl;
}


template<>
identity_list* from_json<identity_list*>(const js::Value& v)
{
	const js::Array& a = v.get_array();

	identity_list* il = new_identity_list( nullptr );

	for(const js::Value& v : a )
	{
		pEp_identity* id = from_json<pEp_identity*>(v);
		identity_list_add(il, id );
	}

	return il;
}


template<>
js::Value to_json<stringlist_t*>(stringlist_t* const& osl)
{
	stringlist_t* sl = osl;
	js::Array a;
	
	while(sl)
	{
		const std::string value = sl->value;
		a.push_back(value);
		sl = sl->next;
	}
	
	return js::Value( a );
}


template<>
js::Value to_json<pEp_identity*>(pEp_identity* const& id)
{
	if(id == nullptr)
	{
		return js::Value("NULL-IDENTITY");
	}

	js::Object o;
	
	o.emplace_back( "struct_size", js::Value( uint64_t( id->struct_size) ));
	to_json_object(o, "address", id->address);
	to_json_object(o, "fpr", id->fpr);
	to_json_object(o, "user_id", id->user_id);
	to_json_object(o, "username", id->username);

	o.emplace_back( "comm_type", js::Value( int( id->comm_type) ));
	o.emplace_back( "lang", js::Value( std::string( id->lang, id->lang+2) ));
	o.emplace_back( "me", js::Value( id->me ));
	
	return js::Value( o );
}


template<>
js::Value to_json<identity_list*>(identity_list* const& idl)
{
	identity_list* il = idl;
	js::Array a;
	
	while(il)
	{
		const js::Value value = to_json<pEp_identity*>(il->ident);
		a.push_back(value);
		il = il->next;
	}
	
	return js::Value( a );
}


template<>
js::Value to_json<PEP_color>(const PEP_color& color)
{
	js::Object o;
	o.emplace_back( "color", int(color) );
	return o;
}


template<>
js::Value to_json<PEP_STATUS>(const PEP_STATUS& status)
{
	js::Object o;
	o.emplace_back( "status", int(status) );
	return o;
}


template<>
PEP_enc_format from_json<PEP_enc_format>(const js::Value& v)
{
	return  PEP_enc_format(v.get_int());
}


template<>
js::Value Type2String<PEP_SESSION>::get()  { return "Session"; }

template<>
js::Value Type2String<_message*>::get()  { return "Message"; }

template<>
js::Value Type2String<pEp_identity*>::get()  { return "Identity"; }

template<>
js::Value Type2String<_stringlist_t*>::get()  { return "StringList"; }

template<>
js::Value Type2String<_PEP_color>::get()  { return "PEP_color"; }

template<>
js::Value Type2String<_PEP_enc_format>::get()  { return "PEP_enc_format"; }

template<>
js::Value Type2String<PEP_STATUS>::get()  { return "PEP_STATUS"; }

