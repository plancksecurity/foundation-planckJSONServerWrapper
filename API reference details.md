### Detailed Function reference for the p≡p JSON Server Adapter. Version “(29) Wenden” ###
Output parameters are denoted by a  **⇑** , InOut parameters are denoted by a  **⇕**  after the parameter type.

Nota bene: This list was created manually from the "authorative API description" and might be outdated.

#### Message API ####

##### MIME_encrypt_message( String mimetext, Integer size, StringList extra, String⇑ mime_ciphertext, PEP_enc_format env_format, Integer flags)

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


##### MIME_encrypt_message_for_self(Identity target_id, String mimetext, Integer size, String⇑ mime_ciphertext, PEP_enc_format enc_format, Integer flags)

encrypt MIME message for user's identity only,  ignoring recipients and other identities from
the message, with MIME output.

```
  parameters:
      target_id (in)          self identity this message should be encrypted for
      mimetext (in)           MIME encoded text to encrypt
      size (in)               size of input mime text
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


##### MIME_decrypt_message(String, Integer, String⇑, StringList⇑, PEP_rating⇑, Integer⇑ )

decrypt a MIME message, with MIME output
```
  parameters:
      mimetext (in)           MIME encoded text to decrypt
      size (in)               size of mime text to decode (in order to decrypt)
      mime_plaintext (out)    decrypted, encoded message
      keylist (out)           stringlist with keyids
      rating (out)            rating for the message
      flags (out)             flags to signal special decryption features

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
```


##### MIME_encrypt_message_ex(String, Integer, StringList, Bool, String⇑, PEP_enc_format, Integer )
(deprecated)

##### MIME_decrypt_message_ex(String, Integer, Bool, String⇑, StringList⇑, PEP_rating⇑, Integer⇑ )
(deprecated)

##### startKeySync()
Start Key Synchronization for the current session.

##### stopKeySync()
Stop Key Synchronization for the current session.

##### startKeyserverLookup()
Start a global thread for Keyserver Lookup. This thread handles all keyserver communication for all sessions.

##### stopKeyserverLookup()
Stop the global thread for Keyserver Lookup.


##### encrypt_message(Message, StringList, Message⇑, PEP_enc_format, Integer)
encrypt message in memory
```
  parameters:
      src (in)            message to encrypt
      extra (in)          extra keys for encryption
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

##### encrypt_message_for_self(Identity, Message, Message⇑, PEP_enc_format, Integer)
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


##### decrypt_message(Message, Message⇑, StringList⇑, PEP_rating⇑, Integer⇑)
decrypt message in memory
```
  parameters:
      src (in)            message to decrypt
      dst (out)           pointer to new decrypted message or NULL on failure
      keylist (out)       stringlist with keyids
      rating (out)        rating for the message
      flags (out)         flags to signal special decryption features

  return value:
      error status 
      or PEP_DECRYPTED if message decrypted but not verified
      or PEP_STATUS_OK on success

 caveat:
      if src is unencrypted this function returns PEP_UNENCRYPTED and sets
      dst to NULL
```

##### outgoing_message_rating(Message, PEP_rating⇑)
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

##### re_evaluate_message_rating(Message, StringList, PEP_rating, PEP_rating⇑)
re-evaluate already decrypted message rating
```
  parameters:
      msg (in)                message to get the rating for
      x_keylist (in)          decrypted message recipients keys fpr
      x_enc_status (in)       original rating for the decrypted message
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

##### identity_rating(Identity, PEP_rating⇑)
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

#### pEp Engine Core API ####
##### log_event(String, String, String, String)

##### get_trustwords(Identity, Identity, Language, String⇑, Integer⇑, Bool)

##### get_languagelist(String⇑)

##### get_phrase(Language, Integer, String⇑)

##### get_engine_version | String | )

##### config_passive_mode | Void | Bool)

##### config_unencrypted_subject | Void | Bool)


#### Identity Management API ####
##### get_identity(String, String, Identity⇑)

##### set_identity(Identity)

##### mark_as_comprimized(String)

##### identity_rating(Identity, PEP_rating⇑)

##### outgoing_message_rating(Message, PEP_rating⇑)

##### set_identity_flags(Identity, Integer)

##### unset_identity_flags(Identity, Integer)


#### Low level Key Management API ####
##### generate_keypair(Identity⇕)

##### delete_keypair(String)

##### import_key(String, Integer, IdentityList⇑)

##### export_key(String, String⇑, Integer⇑)

##### find_keys(String, StringList⇑)

##### get_trust(Identity⇕)

##### own_key_is_listed(String, Bool⇑)

##### own_identities_retrieve(IdentityList⇑)

##### myself(Identity⇕)

##### update_dentity(Identity⇕)

##### trust_personal_key(Identity)

##### key_mistrusted(Identity)

##### key_reset_trust(Identity)

##### least_trust(String, PEP_comm_type⇑)

##### get_key_rating(String, PEP_comm_type⇑)

##### renew_key(String, Timestamp)

##### revoke(String, String)

##### key_expired(String, Integer, Bool⇑)


#### from blacklist.h & OpenPGP_compat.h ####
##### blacklist_add(String)

##### blacklist_delete(String)

##### blacklist_is_listed(String, Bool⇑)

##### blacklist_retrieve(StringList⇑)

##### OpenPGP_list_keyinfo(String, StringPairList⇑)


#### Event Listener & Results ####
##### registerEventListener(String, Integer, String)

##### unregisterEventListener(String, Integer, String)

##### deliverHandshakeResult(Identity, PEP_sync_handshake_result)


#### Other ####
##### version()

##### apiVersion()

##### getGpgEnvironment()


