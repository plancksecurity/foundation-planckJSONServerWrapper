#include "pep-types.hh"
#include "json_spirit/json_spirit_utils.h"

namespace
{
	// fetch a member from the given object, if set. (return a sensible NULL/default value if not)
	template<class T, js::Value_type VALUE_TYPE>
	T from_json_object(const js::Object& obj, const std::string& key)
	{
		const auto v = find_value(obj, key);
		if(v.type() == js::null_type) return T{};
		if(v.type() == VALUE_TYPE) return from_json<T>(v);
		
		throw std::runtime_error("JSON object has a member for key \"" + key + "\""
			" with incompatible type " + js::value_type_to_string( v.type())
			+ " instead of expected type " + js::value_type_to_string(VALUE_TYPE)
			);
	}
	
	template<class T>
	void to_json_object(js::Object& obj, const std::string& key, const T& value)
	{
		if(value!=T{})
		{
			obj.emplace_back( key, js::Value( to_json<T>(value) ));
		}
	}

}


// in pEpEngine.h positive values are hex, negative are decimal. :-o
std::string status_to_string(PEP_STATUS status)
{
	if(status==PEP_STATUS_OK)
		return "PEP_STATUS_OK";
	
	std::stringstream ss;
	if(status>0)
	{
		ss << "0x" << std::hex << status;
	}else{
		ss << status;
	}
	return ss.str();
}


template<>
In<const timestamp*>::~In()
{
	free_timestamp(const_cast<timestamp*>(value)); // FIXME: is that const_cast okay?
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
Out<pEp_identity*>::~Out()
{
	if(value)
	{
		free_identity(*value);
	}
	delete value;
}

template<>
Out<identity_list*>::~Out()
{
	if(value)
	{
		free_identity_list(*value);
	}
	delete value;
}


template<>
In<pEp_identity*>::In(const In<pEp_identity*>& other)
: value( other.value ? identity_dup(other.value) : nullptr )
{
}

template<>
Out<pEp_identity*>::Out(const Out<pEp_identity*>& other)
: value( new pEp_identity*{} )
{
	if(*other.value)
	{
		*value = identity_dup(*other.value);
	}
}


template<>
Out<_identity_list*>::Out(const Out<identity_list*>& other)
: value( new identity_list*{} )
{
	if(*other.value)
	{
		*value = identity_list_dup(*other.value);
	}
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
///////////////////////////////////////////////////////////////////////////////
//  FIXME: due to memory corruption(?) the free_message() call crashes! :-(  //
//  Without it we leak memory but at least it works for now... :-/           //
///////////////////////////////////////////////////////////////////////////////
//	if(value) free_message(*value);
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
Out<PEP_comm_type>::Out(const Out<PEP_comm_type>& other)
: value( new PEP_comm_type(*other.value) )
{
}

template<>
Out<PEP_comm_type>::~Out()
{
	delete value;
}


template<>
message* from_json<message*>(const js::Value& v)
{
	const js::Object& o = v.get_obj();

	_message* msg = new_message(PEP_dir_incoming);

	// fetch values from v and put them into msg
	msg->dir      = from_json_object<PEP_msg_direction, js::int_type>(o, "dir");
	msg->id       = from_json_object<char*, js::str_type>(o, "id");
	msg->shortmsg = from_json_object<char*, js::str_type>(o, "shortmsg");
	msg->longmsg  = from_json_object<char*, js::str_type>(o, "longmsg");
	msg->longmsg_formatted = from_json_object<char*, js::str_type>(o, "longmsg_formatted");
	
	msg->attachments = from_json_object<bloblist_t*, js::array_type>(o, "attachments");
	
	msg->sent     = from_json_object<timestamp*, js::int_type>(o, "sent");
	msg->recv     = from_json_object<timestamp*, js::int_type>(o, "recv");
	
	msg->from     = from_json_object<pEp_identity*, js::obj_type>(o, "from");
	msg->to       = from_json_object<identity_list*, js::array_type>(o, "to");
	msg->recv_by  = from_json_object<pEp_identity*, js::obj_type>(o, "recv_by");
	msg->cc       = from_json_object<identity_list*, js::array_type>(o, "cc");
	msg->bcc      = from_json_object<identity_list*, js::array_type>(o, "bcc");
	msg->reply_to = from_json_object<identity_list*, js::array_type>(o, "reply_to");
	msg->in_reply_to = from_json_object<stringlist_t*, js::array_type>(o, "in_reply_to");
	
	// TODO: refering_msg_ref
	msg->references = from_json_object<stringlist_t*, js::array_type>(o, "references");
	// TODO: refered_by
	
	msg->keywords = from_json_object<stringlist_t*, js::int_type>(o, "keywords");
	msg->comments = from_json_object<char*, js::str_type>(o, "comments");
	msg->opt_fields = from_json_object<stringpair_list_t*, js::array_type>(o, "opt_fields");
	msg->enc_format = from_json_object<PEP_enc_format, js::int_type>(o, "enc_format");
	
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
	char* lang        = from_json_object<char*, js::str_type>(o, "lang");
	
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
	
	ident->comm_type = from_json_object<PEP_comm_type, js::int_type>(o, "comm_type");
	if(lang && lang[0] && lang[1])
	{
		strncpy(ident->lang, lang, 3);
		free(lang);
	}
	ident->me = from_json_object<bool, js::bool_type>(o, "me");
	
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
	to_json_object(o, "dir"     , msg->dir);
	to_json_object(o, "id"      , msg->id);
	to_json_object(o, "shortmsg", msg->shortmsg);
	to_json_object(o, "longmsg" , msg->longmsg);
	to_json_object(o, "longmsg_formatted"  , msg->longmsg_formatted);
	to_json_object(o, "attachments", msg->attachments);
	to_json_object(o, "sent"    , msg->sent);
	to_json_object(o, "recv"    , msg->recv);
	
	to_json_object(o, "from"    , msg->from);
	to_json_object(o, "to"      , msg->to);
	to_json_object(o, "recv_by" , msg->recv_by);
	
	to_json_object(o, "cc"      , msg->cc);
	to_json_object(o, "bcc"     , msg->bcc);
	to_json_object(o, "reply_to", msg->reply_to);
	to_json_object(o, "in_reply_to"   , msg->in_reply_to);
	
	// TODO: refering_msg_ref
	to_json_object(o, "references", msg->references);
	// TODO: refered_by
	
	to_json_object(o, "keywords", msg->keywords);
	to_json_object(o, "comments", msg->comments);
	to_json_object(o, "opt_fields", msg->opt_fields);
	to_json_object(o, "enc_format", msg->enc_format);
	
	return js::Value( std::move(o) );
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
_bloblist_t* from_json<_bloblist_t*>(const js::Value& v)
{
	const js::Array& a = v.get_array();
	if(a.empty())
		return nullptr;
	
	auto element = a.begin();
	const auto oelem = element->get_obj();
	_bloblist_t* bl = new_bloblist
		(
			from_json_object<char*, js::str_type>      (oelem, "data"),
			from_json_object<size_t, js::int_type>     (oelem, "size"),
			from_json_object<const char*, js::str_type>(oelem, "mime_type"),
			from_json_object<const char*, js::str_type>(oelem, "filename")
		);

	for(; element!=a.end(); ++element)
	{
		const auto oelem = element->get_obj();
		bl = bloblist_add(bl, 
			from_json_object<char*, js::str_type>      (oelem, "data"),
			from_json_object<size_t, js::int_type>     (oelem, "size"),
			from_json_object<const char*, js::str_type>(oelem, "mime_type"),
			from_json_object<const char*, js::str_type>(oelem, "filename")
		);
	}

	return bl;
}


template<>
js::Value to_json<_bloblist_t*>(_bloblist_t* const& bl)
{
	_bloblist_t* b = bl;
	js::Array a;
	
	while(b)
	{
		js::Object o;
		if(b->value)
			o.emplace_back( "value", std::string(b->value, b->value + b->size) );
		
		o.emplace_back( "size", b->size );
		
		if(b->mime_type)
			o.emplace_back( "mime_type", b->mime_type );
		
		if(b->filename)
			o.emplace_back( "filename", b->filename );
		
		a.push_back( std::move(o) );
		b = b->next;
	}
	
	return js::Value( std::move(a) );
}

template<>
stringpair_t* from_json<stringpair_t*>(const js::Value& v)
{
	const js::Object& o = v.get_obj();
	char* key = from_json_object<char*, js::str_type>(o, "key");
	char* val = from_json_object<char*, js::str_type>(o, "value");
	stringpair_t* sp = new_stringpair( key, val );
	free(val);
	free(key);
	return sp;
}



template<>
stringpair_list_t* from_json<stringpair_list_t*>(const js::Value& v)
{
	const js::Array& a = v.get_array();
	if(a.empty())
		return nullptr;
	
	auto element = a.begin();
	stringpair_list_t* spl = new_stringpair_list( from_json<stringpair_t*>(*element) );
	
	++element;
	
	for(; element!=a.end(); ++element)
	{
		spl = stringpair_list_add(spl, from_json<stringpair_t*>(*element) );
	}

	return spl;
}


template<>
js::Value to_json<stringpair_list_t*>(stringpair_list_t* const& osl)
{
	stringpair_list_t* spl = osl;
	js::Array a;
	
	while(spl)
	{
		js::Object o;
		o.emplace_back( "key", spl->value->key );
		o.emplace_back( "value", spl->value->value );
		a.push_back( std::move(o) );
		spl = spl->next;
	}
	
	return js::Value( std::move(a) );
}


template<>
tm* from_json<tm*>(const js::Value& v)
{
	return new_timestamp( v.get_int64() );
}

template<>
const tm* from_json<const tm*>(const js::Value& v)
{
	return new_timestamp( v.get_int64() );
}


template<>
js::Value to_json<stringlist_t*>(stringlist_t* const& osl)
{
	stringlist_t* sl = osl;
	js::Array a;
	
	while(sl)
	{
		std::string value = sl->value;
		a.push_back( std::move(value) );
		sl = sl->next;
	}
	
	return js::Value( std::move(a) );
}


template<>
js::Value to_json<pEp_identity*>(pEp_identity* const& id)
{
	if(id == nullptr)
	{
		return js::Value("NULL-IDENTITY");
	}

	js::Object o;
	
	to_json_object(o, "address", id->address);
	to_json_object(o, "fpr", id->fpr);
	to_json_object(o, "user_id", id->user_id);
	to_json_object(o, "username", id->username);

	o.emplace_back( "comm_type", js::Value( int( id->comm_type) ));
	
	if(id->lang && id->lang[0] && id->lang[1])
		o.emplace_back( "lang", js::Value( std::string( id->lang, id->lang+2) ));
	
	o.emplace_back( "me", js::Value( id->me ));
	
	return js::Value( std::move(o) );
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
	
	return js::Value( std::move(a) );
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
	o.emplace_back( "hex", status_to_string(status) );
	return o;
}


template<>
PEP_enc_format from_json<PEP_enc_format>(const js::Value& v)
{
	return  PEP_enc_format(v.get_int());
}

template<>
js::Value to_json<PEP_enc_format>(const PEP_enc_format& v)
{
	return js::Value( int(v) );
}


template<>
PEP_msg_direction from_json<PEP_msg_direction>(const js::Value& v)
{
	return  PEP_msg_direction(v.get_int());
}


template<>
js::Value to_json<PEP_msg_direction>(const PEP_msg_direction& v)
{
	return js::Value( int(v) );
}

template<>
PEP_comm_type from_json<PEP_comm_type>(const js::Value& v)
{
	return  PEP_comm_type(v.get_int());
}

template<>
js::Value to_json<PEP_comm_type>(const PEP_comm_type& v)
{
	return js::Value( int(v) );
}


template<>
js::Value Type2String<PEP_SESSION>::get()  { return "Session"; }

template<>
js::Value Type2String<_message*>::get()  { return "Message"; }

template<>
js::Value Type2String<const timestamp*>::get()  { return "Timestamp"; }

template<>
js::Value Type2String<pEp_identity*>::get()  { return "Identity"; }

template<>
js::Value Type2String<identity_list*>::get()  { return "IdentityList"; }

template<>
js::Value Type2String<_stringlist_t*>::get()  { return "StringList"; }

template<>
js::Value Type2String<_PEP_color>::get()  { return "PEP_color"; }

template<>
js::Value Type2String<_PEP_enc_format>::get()  { return "PEP_enc_format"; }

template<>
js::Value Type2String<_PEP_comm_type>::get()  { return "PEP_comm_type"; }

template<>
js::Value Type2String<PEP_STATUS>::get()  { return "PEP_STATUS"; }

