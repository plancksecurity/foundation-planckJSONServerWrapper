### Detailed Function reference for the p≡p JSON Server Adapter. Version “(38) Frankenberg”, API version 0.15.0 ###
Output parameters are denoted by a  **⇑** , InOut parameters are denoted by a  **⇕**  after the parameter type.

Nota bene: This list was created manually from the "authorative API description" and might be outdated.

#### Message API ####

##### MIME_encrypt_message( String mimetext, Integer size, StringList extra, String⇑ mime_ciphertext, PEP_enc_format env_format, Integer flags) #####

encrypt a MIME message, with MIME output

```
  parameters:
      mimetext (in)           MIME encoded text to encrypt
      size (in)               size of input mime text
      extra (in)              extra keys for encryption
      mime_ciphertext (out)   encrypted, encoded message
      enc_format (in)         encrypted format
      flags (in)              flags to set special encryption features

  return value:
      PEP_STATUS_OK           if everything worked
      PEP_BUFFER_TOO_SMALL    if encoded message size is too big to handle
      PEP_CANNOT_CREATE_TEMP_FILE
                              if there are issues with temp files; in
                              this case errno will contain the underlying
                              error
      PEP_OUT_OF_MEMORY       if not enough memory could be allocated
```
*Caveat:* the encrypted, encoded mime text will go to the ownership of the caller; mimetext
will remain in the ownership of the caller


##### MIME_encrypt_message_for_self( Identity target_id, String mimetext, Integer size, StringList extra, String⇑ mime_ciphertext, PEP_enc_format enc_format, Integer flags) #####
encrypt MIME message for user's identity only,  ignoring recipients and other identities from
the message, with MIME output

```
  parameters:
      target_id (in)          self identity this message should be encrypted for
      mimetext (in)           MIME encoded text to encrypt
      size (in)               size of input mime text
      extra (in)              extra keys for encryption
      mime_ciphertext (out)   encrypted, encoded message
      enc_format (in)         encrypted format
      flags (in)              flags to set special encryption features

  return value:
      PEP_STATUS_OK           if everything worked
      PEP_BUFFER_TOO_SMALL    if encoded message size is too big to handle
      PEP_CANNOT_CREATE_TEMP_FILE
                              if there are issues with temp files; in
                              this case errno will contain the underlying
                              error
      PEP_OUT_OF_MEMORY       if not enough memory could be allocated

  caveat:
      the encrypted, encoded mime text will go to the ownership of the caller; mimetext
      will remain in the ownership of the caller
```


##### MIME_decrypt_message(String mimetext, Integer size, String⇑ mime_plaintext, StringList⇑ keylist, PEP_rating⇑ rating, Integer⇕ flags, String⇑ modified_src)

decrypt a MIME message, with MIME output
```
  parameters:
      mimetext (in)           MIME encoded text to decrypt
      size (in)               size of mime text to decode (in order to decrypt)
      mime_plaintext (out)    decrypted, encoded message
      keylist (out)           stringlist with keyids
      rating (out)            rating for the message
      flags (inout)           flags to signal special decryption features
      modified_src (out)      modified source string, if decrypt had reason to change it

  return value:
      decrypt status          if everything worked with MIME encode/decode, 
                              the status of the decryption is returned 
                              (PEP_STATUS_OK or decryption error status)
      PEP_BUFFER_TOO_SMALL    if encoded message size is too big to handle
      PEP_CANNOT_CREATE_TEMP_FILE
                              if there are issues with temp files; in
                              this case errno will contain the underlying
                              error
      PEP_OUT_OF_MEMORY       if not enough memory could be allocated

  flag values:
      in:
          PEP_decrypt_flag_untrusted_server
              used to signal that decrypt function should engage in behaviour
              specified for when the server storing the source is untrusted.
      out:
          PEP_decrypt_flag_own_private_key
              private key was imported for one of our addresses (NOT trusted
              or set to be used - handshake/trust is required for that)
          PEP_decrypt_flag_src_modified
              indicates that the modified_src field should contain a modified
              version of the source, at the moment always as a result of the
              input flags. 
          PEP_decrypt_flag_consume
              used by sync 
          PEP_decrypt_flag_ignore
              used by sync 
 
  caveat:
      the decrypted, encoded mime text will go to the ownership of the caller; mimetext
      will remain in the ownership of the caller
```


##### startKeySync()
Start Key Synchronization for the current session.

##### stopKeySync()
Stop Key Synchronization for the current session.

##### startKeyserverLookup()
Start a global thread for Keyserver Lookup. This thread handles all keyserver communication for all sessions.

##### stopKeyserverLookup()
Stop the global thread for Keyserver Lookup.


##### encrypt_message(Message src, StringList extra_keys, Message⇑ dst, PEP_enc_format enc_format, Integer flags)
encrypt message in memory
```
  parameters:
      src (in)            message to encrypt
      extra_keys (in)     extra keys for encryption
      dst (out)           pointer to new encrypted message or NULL on failure
      enc_format (in)     encrypted format
      flags (in)          flags to set special encryption features

  return value:
      PEP_STATUS_OK                   on success
      PEP_KEY_NOT_FOUND               at least one of the receipient keys
                                      could not be found
      PEP_KEY_HAS_AMBIG_NAME          at least one of the receipient keys has
                                      an ambiguous name
      PEP_GET_KEY_FAILED              cannot retrieve key
      PEP_UNENCRYPTED                 no recipients with usable key, 
                                      message is left unencrypted,
                                      and key is attached to it
```

##### encrypt_message_and_add_priv_key( Message src, Message⇑ dst, String to_fingerprint, PEP_enc_format enc_format, Integer flags)
encrypt message in memory, adding an encrypted private key (encrypted separately and sent within the inner message)
```
  parameters:
      session (in)        session handle
      src (in)            message to encrypt
      dst (out)           pointer to new encrypted message or NULL if no
                          encryption could take place
      to_fpr              fingerprint of the recipient key to which the private key
                          should be encrypted
      enc_format (in)     encrypted format
      flags (in)          flags to set special encryption features

  return value:
      PEP_STATUS_OK                   on success
      PEP_KEY_HAS_AMBIG_NAME          at least one of the receipient keys has
                                      an ambiguous name
      PEP_UNENCRYPTED                 on demand or no recipients with usable
                                      key, is left unencrypted, and key is
                                      attached to it

  caveat:
      the ownershop of src remains with the caller
      the ownership of dst goes to the caller
```

##### encrypt_message_for_self(Identity target_id, Message src, Message⇑ dst, PEP_enc_format enc_format, Integer flags)
encrypt message in memory for user's identity only,
ignoring recipients and other identities from
the message.

```
  parameters:
      target_id (in)      self identity this message should be encrypted for
      src (in)            message to encrypt
      dst (out)           pointer to new encrypted message or NULL on failure
      enc_format (in)     encrypted format
      flags (in)          flags to set special encryption features

  return value:       (FIXME: This may not be correct or complete)
      PEP_STATUS_OK            on success
      PEP_KEY_NOT_FOUND        at least one of the receipient keys
                               could not be found
      PEP_KEY_HAS_AMBIG_NAME   at least one of the receipient keys has
                               an ambiguous name
      PEP_GET_KEY_FAILED       cannot retrieve key
```
*Caveat:* message is NOT encrypted for identities other than the target_id (and then,
only if the target_id refers to self!)


##### decrypt_message(Message⇕ src, Message⇑ dst, StringList⇑ keylist, PEP_rating⇑ rating, Integer⇕ flags)
decrypt message in memory
```
  parameters:
      src (inout)         message to decrypt
      dst (out)           pointer to new decrypted message or NULL on failure
      keylist (out)       stringlist with keyids
      rating (out)        rating for the message
      flags (inout)       flags to signal special decryption features

  return value:
      error status 
      or PEP_DECRYPTED if message decrypted but not verified
      or PEP_CANNOT_REENCRYPT if message was decrypted (and possibly
         verified) but a reencryption operation is expected by the caller
         and failed
      or PEP_STATUS_OK on success

  flag values:
      in:
          PEP_decrypt_flag_untrusted_server
              used to signal that decrypt function should engage in behaviour
              specified for when the server storing the source is untrusted
      out:
          PEP_decrypt_flag_own_private_key
              private key was imported for one of our addresses (NOT trusted
              or set to be used - handshake/trust is required for that)
          PEP_decrypt_flag_src_modified
              indicates that the src object has been modified. At the moment,
              this is always as a direct result of the behaviour driven
              by the input flags. This flag is the ONLY value that should be
              relied upon to see if such changes have taken place.
          PEP_decrypt_flag_consume
              used by sync 
          PEP_decrypt_flag_ignore
              used by sync 


 caveat:
      the ownership of src remains with the caller - however, the contents 
          might be modified (strings freed and allocated anew or set to NULL,
          etc) intentionally; when this happens, PEP_decrypt_flag_src_modified
          is set.
      the ownership of dst goes to the caller
      the ownership of keylist goes to the caller
      if src is unencrypted this function returns PEP_UNENCRYPTED and sets
         dst to NULL
```

##### outgoing_message_rating(Message msg, PEP_rating⇑ rating)
get rating for an outgoing message
```
  parameters:
      msg (in)            message to get the rating for
      rating (out)        rating for the message

  return value:
      error status or PEP_STATUS_OK on success

  caveat:
      msg->from must point to a valid pEp_identity
      msg->dir must be PEP_dir_outgoing
```

##### re_evaluate_message_rating(Message msg, StringList keylist, PEP_rating enc_status, PEP_rating⇑ rating)
re-evaluate already decrypted message rating
```
  parameters:
      msg (in)                message to get the rating for
      keylist (in)            decrypted message recipients keys fpr
      enc_status (in)         original rating for the decrypted message
      rating (out)            rating for the message

  return value:
      PEP_ILLEGAL_VALUE       if decrypted message doesn't contain 
                              X-EncStatus optional field and x_enc_status is 
                              pEp_rating_udefined
                              or if decrypted message doesn't contain 
                              X-Keylist optional field and x_keylist is NULL
      PEP_OUT_OF_MEMORY       if not enough memory could be allocated

  caveat:
      msg->from must point to a valid pEp_identity
```

##### identity_rating(Identity ident, PEP_rating⇑ rating)
 get rating for a single identity
```
  parameters:
      ident (in)          identity to get the rating for
      rating (out)        rating for the identity

  return value:
      error status or PEP_STATUS_OK on success
```

##### get_gpg_path(String⇑)
get path of gpg binary.

#### MIME message handling ####

##### mime_decode_message(String mime_message, Integer message_lenght, Message msg)

#### pEp Engine Core API ####

##### get_trustwords(Identity id1, Identity id2, Language lang, String⇑ words, Integer⇑ wsize, Bool full)
get full trustwords string for a *pair* of identities
```
    parameters:
        id1 (in)            identity of first party in communication - fpr can't be NULL  
        id2 (in)            identity of second party in communication - fpr can't be NULL
        lang (in)           C string with ISO 639-1 language code
        words (out)         pointer to C string with all trustwords UTF-8 encoded,
                            separated by a blank each
                            NULL if language is not supported or trustword
                            wordlist is damaged or unavailable
        wsize (out)         length of full trustwords string
        full (in)           if true, generate ALL trustwords for these identities.
                            else, generate a fixed-size subset. (TODO: fixed-minimum-entropy
                            subset in next version)

    return value:
        PEP_STATUS_OK            trustwords retrieved
        PEP_OUT_OF_MEMORY        out of memory
        PEP_TRUSTWORD_NOT_FOUND  at least one trustword not found
```

##### get_languagelist(String⇑ languages)
get the list of languages
```
  parameters:
      languages (out)         languages as string in double quoted CSV format
                              column 1 is the ISO 639-1 language code
                              column 2 is the name of the language
```


##### is_pep_user(Identity id, Bool⇑  ia_pwp)
returns true if the USER corresponding to this identity has been listed in the *person* table as a pEp user
```
parameters:
    identity (in) - identity containing the user_id to check (this is
                    the only part of the struct we require to be set)
    is_pep (out)  - boolean pointer - will return true or false by
                    reference with respect to whether or not user is
                    a known pep user
```

##### config_passive_mode(Bool enable)
enable passive mode

*  parameters:   enable (in)     flag if enabled or disabled


##### config_unencrypted_subject(Bool enable)
disable subject encryption

* parameters:  enable (in)     flag if enabled or disabled


#### Identity Management API ####
##### get_identity(String address, String user_id, Identity⇑ identity)
get identity information

```
    parameters:
        address (in)        string with communication address, UTF-8 encoded
        user_id (in)        unique string to identify person that identity is refering to
        identity (out)      pEp_identity structure with results or NULL if failure
```

##### set_identity(Identity)
set identity information
```
    parameters:
        identity (in)       pEp_identity structure

    return value:
        PEP_STATUS_OK = 0             encryption and signing succeeded
        PEP_CANNOT_SET_PERSON         writing to table person failed
        PEP_CANNOT_SET_PGP_KEYPAIR    writing to table pgp_keypair failed
        PEP_CANNOT_SET_IDENTITY       writing to table identity failed
        PEP_COMMIT_FAILED             SQL commit failed
        PEP_KEY_BLACKLISTED           Key blacklisted, cannot set identity

    caveat:
        address, fpr, user_id and username must be given
```

##### mark_as_comprimized(String fpr)
mark key in trust db as compromized

* parameters:  fpr (in)            fingerprint of key to mark


##### identity_rating(Identity ident, PEP_rating⇑ rating)
get rating for a single identity
```
  parameters:
      ident (in)          identity to get the rating for
      rating (out)        rating for the identity

  return value:
      error status or PEP_STATUS_OK on success
```

##### outgoing_message_rating(Message msg, PEP_rating⇑ rating)
get rating for an outgoing message
```
  parameters:
      msg (in)            message to get the rating for
      rating (out)        rating for the message

  return value:
      error status or PEP_STATUS_OK on success

  caveat:
      msg->from must point to a valid pEp_identity
      msg->dir must be PEP_dir_outgoing
```

##### set_identity_flags(Identity⇕ identity, Integer flags)
update identity flags on existing identity
```
    parameters:
        identity (in,out)   pointer to pEp_identity structure
        flags (in)          new value for flags

    return value:
        PEP_STATUS_OK = 0             encryption and signing succeeded
        PEP_CANNOT_SET_IDENTITY       update of identity failed

    caveat:
        address and user_id must be given in identity
```

##### unset_identity_flags(Identity⇕ identity, Integer flags)
update identity flags on existing identity
```
    parameters:
        identity (in,out)   pointer to pEp_identity structure
        flags (in)          new value for flags

    return value:
        PEP_STATUS_OK = 0             encryption and signing succeeded
        PEP_CANNOT_SET_IDENTITY       update of identity failed

    caveat:
        address and user_id must be given in identity
```

#### Low level Key Management API ####
##### generate_keypair(Identity⇕ identity)
generate a new key pair and add it to the key ring
```
  parameters:
        identity (inout)      pEp_identity structure

    return value:
        PEP_STATUS_OK = 0       encryption and signing succeeded
        PEP_ILLEGAL_VALUE       illegal values for identity fields given
        PEP_CANNOT_CREATE_KEY   key engine is on strike

  caveat:
      address and username fields must be set to UTF-8 strings
      the fpr field must be set to NULL
```

##### delete_keypair(String fpr)
delete a public key or a key pair from the key ring
```
  parameters:
      fpr (in)                string with key id or fingerprint of the public key

  return value:
      PEP_STATUS_OK = 0       key was successfully deleted
      PEP_KEY_NOT_FOUND       key not found
      PEP_ILLEGAL_VALUE       not a valid key id or fingerprint
      PEP_KEY_HAS_AMBIG_NAME  fpr does not uniquely identify a key
      PEP_OUT_OF_MEMORY       out of memory
```

##### import_key(String key_data, Integer size, IdentityList⇑ private_keys)
import key from data
```
  parameters:
      key_data (in)           key data, i.e. ASCII armored OpenPGP key
      size (in)               amount of data to handle
      private_keys (out)      list of private keys that have been imported

  return value:
      PEP_STATUS_OK = 0       key was successfully imported
      PEP_OUT_OF_MEMORY       out of memory
      PEP_ILLEGAL_VALUE       there is no key data to import
```

##### export_key(String fpr, String⇑ key_data, Integer⇑ size)
export ascii armored key
```
  parameters:
      fpr (in)                key id or fingerprint of key
      key_data (out)          ASCII armored OpenPGP key
      size (out)              amount of data to handle

  return value:
      PEP_STATUS_OK = 0       key was successfully exported
      PEP_OUT_OF_MEMORY       out of memory
      PEP_KEY_NOT_FOUND       key not found
```

##### find_keys(String pattern, StringList⇑ keylist)
find keys in keyring
```
  parameters:
      pattern (in)            key id, user id or address to search for as UTF-8 string
      keylist (out)           list of fingerprints found or NULL on error
```

##### get_trust(Identity⇕ identity)
get the trust level a key has for a person

```
  parameters:
      identity (inout)        user_id and fpr to check as UTF-8 strings (in)
                              user_id and comm_type as result (out)
```

This function modifies the given identity struct; the struct remains in
the ownership of the caller.
If the trust level cannot be determined identity->comm_type is set
to PEP_ct_unknown.


##### own_key_is_listed(String fpr, Bool⇑ listed)
returns true id key is listed as own key
```
  parameters:
      fpr (in)            fingerprint of key to test
      listed (out)        flags if key is own
```

##### own_identities_retrieve(IdentityList⇑ own_identities)
retrieve all own identities
```
  parameters:
      own_identities (out)    list of own identities
```

##### set_own_key( Identity⇕ id, String fpr)
mark key as own key
```
  parameters:
     me (inout)              own identity this key is used for                                                                    
     fpr (in)                fingerprint of the key to mark as own key                                                            
```

##### undo_last_mistrust()
reset identity and trust status for the last`identity in this session marked
as mistrusted to their cached values from the time of mistrust

```
  parameters:
      (none)

  return value:
      PEP_STATUS_OK if identity and trust were successfully restored.
      Otherwise, error status from attempts to set.

  caveat:
      only works for this session, and only once. cache is invalidated
      upon use.

      WILL NOT WORK ON MISTRUSTED OWN KEY
```

##### myself(Identity⇕ identity)
ensures that the own identity is being complete
```
  parameters:
      identity (inout)    identity of local user. At least .address, .username, .user_id must be set.

  return value:
      PEP_STATUS_OK if identity could be completed or was already complete, any other value on error

  caveat:
      This function generates a keypair on demand; because it's synchronous
      it can need a decent amount of time to return.
      If you need to do this asynchronous, you need to return an identity
      with retrieve_next_identity() where pEp_identity.me is true.
```

##### update_identity(Identity⇕)
update identity information
```
  parameters:
      identity (inout)    identity information of communication partner
                          (identity->fpr is OUT ONLY)
  return value:
      PEP_STATUS_OK if identity could be updated,
      PEP_GET_KEY_FAILED for own identity that must be completed (myself())
      any other value on error

  caveat:
      if this function returns PEP_ct_unknown or PEP_ct_key_expired in
      identity->comm_type, the caller must insert the identity into the
      asynchronous management implementation, so retrieve_next_identity()
      will return this identity later
      at least identity->address must be a non-empty UTF-8 string as input
      update_identity() never writes flags; use set_identity_flags() for
      writing
      this function NEVER reads the incoming fpr, only writes to it.
```

##### trust_personal_key(Identity)
mark a key as trusted with a person
```
  parameters:
      ident (in)          person and key to trust in

  caveat:
      the fields user_id, address and fpr must be supplied
```

##### key_mistrusted(Identity)
mark key as being compromized
```
  parameters:
      ident (in)          person and key which was compromized
```

##### key_reset_trust(Identity)
 undo trust_personal_key and key_mistrusted() for keys we don't own
```
  parameters:
      ident (in)          person and key which was compromized
```

##### least_trust(String fpr, PEP_comm_type⇑ comm_type)
get the least known trust level for a key in the database
```
  parameters:
      fpr (in)                fingerprint of key to check
      comm_type (out)         least comm_type as result (out)
```

If the trust level cannot be determined comm_type is set to PEP_ct_unknown.


##### get_key_rating(String fpr, PEP_comm_type⇑ comm_type)
get the rating a bare key has
```
  parameters:
      fpr (in)                unique identifyer for key as UTF-8 string
      comm_type (out)         key rating
```

Iif an error occurs, *comm_type is set to PEP_ct_unknown and an error is returned


##### renew_key(String fpr, Timestamp ts)
renew an expired key
```
  parameters:
      fpr (in)                ID of key to renew as UTF-8 string
      ts (in)                 timestamp when key should expire or NULL for default
```

##### revoke(String fpr, String reason)
revoke a key
```
  parameters:
      fpr (in)                ID of key to revoke as UTF-8 string
      reason (in)             text with reason for revoke as UTF-8 string
                              or NULL if reason unknown

  caveat:
      reason text must not include empty lines
      this function is meant for internal use only; better use
      key_mistrusted() of keymanagement API
```

##### key_expired(String fpr, Integer when, Bool⇑ expired)
flags if a key is already expired
```
  parameters:
      fpr (in)                ID of key to check as UTF-8 string
      when (in)               UTC time of when should expiry be considered
      expired (out)           flag if key expired
```


#### from blacklist.h & OpenPGP_compat.h ####
##### blacklist_add(String fpr)
add to blacklist
*  parameters:  fpr (in)            fingerprint of key to blacklist


##### blacklist_delete(String fpr)
delete from blacklist
*  parameters:  fpr (in)            fingerprint of key to blacklist

##### blacklist_is_listed(String fpr, Bool⇑ listed)
is_listed in blacklist
```
  parameters:
      session (in)        session to use
      fpr (in)            fingerprint of key to blacklist
      listted (out)
```

##### blacklist_retrieve(StringList⇑ blacklist)
retrieve full blacklist of key fingerprints

* parameters:   blacklist (out)     copy of blacklist


##### OpenPGP_list_keyinfo(String search_pattern, StringPairList⇑ keyinfo_list)
get a key/UID list for pattern matches in keyring ("" to return entire keyring), filtering out revoked keys in the results
```
  parameters:
      search_pattern (in)   search pattern - either an fpr, or something within the UID, or "" for all keys
      keyinfo_list (out)    a key/value pair list for each key / UID combination
```


#### Event Listener & Results ####
##### registerEventListener(String address, Integer port, String security_context)
Register an address/port pair where a JSON-RPC call shall be made to, when the Engine wants to call the client application.
These RPC calls are authenticated with a security_context parameter that is given to all calls (and can be different from the security_context
that is used for calls from the client to the JSON Server Adapter).

Currently there are two functions that can be called:
* messageToSend( Message )
* notifyHandshake( Identity self, Identity partner, sync_handshake_signal sig )

##### unregisterEventListener(String address, Integer port, String security_context)
Unregister a previous registered JSON-RPC listener.

##### deliverHandshakeResult(Identity partner, PEP_sync_handshake_result result)
give the result of the handshake dialog back to the Engine
```
  parameters:
      partner (in)        the parther of the handshake
      result (in)         handshake result
```

#### Other ####

##### serverVersion()
Returns a struct with SemVer-compatible ABI version, the codename of the
JSON Adapter version etc.

##### version()
Returns a codename for the current JSON Server Adapter's version.


##### getGpgEnvironment()
Returns a struct holding 3 members
* gnupg_path
* gnupg_home environment variable, if set
* gpg_agent_info environment variable, if set.

##### shutdown()
shutdown the JSON Adapter
