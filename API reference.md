### Function reference for the p≡p JSON Server Adapter. Version “(30) Krombach” ###
Output parameters are denoted by a  **⇑** , InOut parameters are denoted by a  **⇕**  after the parameter type.

#### Message API ####
| Function name | Return Type | Parameters |
|---------------|-------------|------------|
| MIME_encrypt_message | PEP_STATUS | String, Integer, StringList, String⇑, PEP_enc_format, Integer |
| MIME_encrypt_message_for_self | PEP_STATUS | Identity, String, Integer, String⇑, PEP_enc_format, Integer |
| MIME_decrypt_message | PEP_STATUS | String, Integer, String⇑, StringList⇑, PEP_rating⇑, Integer⇑ |
| MIME_encrypt_message_ex | PEP_STATUS | String, Integer, StringList, Bool, String⇑, PEP_enc_format, Integer |
| MIME_decrypt_message_ex | PEP_STATUS | String, Integer, Bool, String⇑, StringList⇑, PEP_rating⇑, Integer⇑ |
| startKeySync | Void |  |
| stopKeySync | Void |  |
| startKeyserverLookup | Void |  |
| stopKeyserverLookup | Void |  |
| encrypt_message | PEP_STATUS | Message, StringList, Message⇑, PEP_enc_format, Integer |
| encrypt_message_for_self | PEP_STATUS | Identity, Message, Message⇑, PEP_enc_format, Integer |
| decrypt_message | PEP_STATUS | Message, Message⇑, StringList⇑, PEP_rating⇑, Integer⇑ |
| outgoing_message_rating | PEP_STATUS | Message, PEP_rating⇑ |
| re_evaluate_message_rating | PEP_STATUS | Message, StringList, PEP_rating, PEP_rating⇑ |
| identity_rating | PEP_STATUS | Identity, PEP_rating⇑ |
| get_gpg_path | PEP_STATUS | String⇑ |


#### pEp Engine Core API ####
| Function name | Return Type | Parameters |
|---------------|-------------|------------|
| log_event | PEP_STATUS | String, String, String, String |
| get_trustwords | PEP_STATUS | Identity, Identity, Language, String⇑, Integer⇑, Bool |
| get_languagelist | PEP_STATUS | String⇑ |
| get_phrase | PEP_STATUS | Language, Integer, String⇑ |
| get_engine_version | String |  |
| config_passive_mode | Void | Bool |
| config_unencrypted_subject | Void | Bool |


#### Identity Management API ####
| Function name | Return Type | Parameters |
|---------------|-------------|------------|
| get_identity | PEP_STATUS | String, String, Identity⇑ |
| set_identity | PEP_STATUS | Identity |
| mark_as_comprimized | PEP_STATUS | String |
| identity_rating | PEP_STATUS | Identity, PEP_rating⇑ |
| outgoing_message_rating | PEP_STATUS | Message, PEP_rating⇑ |
| set_identity_flags | PEP_STATUS | Identity⇕, Integer |
| unset_identity_flags | PEP_STATUS | Identity⇕, Integer |


#### Low level Key Management API ####
| Function name | Return Type | Parameters |
|---------------|-------------|------------|
| generate_keypair | PEP_STATUS | Identity⇕ |
| delete_keypair | PEP_STATUS | String |
| import_key | PEP_STATUS | String, Integer, IdentityList⇑ |
| export_key | PEP_STATUS | String, String⇑, Integer⇑ |
| find_keys | PEP_STATUS | String, StringList⇑ |
| get_trust | PEP_STATUS | Identity⇕ |
| own_key_is_listed | PEP_STATUS | String, Bool⇑ |
| own_identities_retrieve | PEP_STATUS | IdentityList⇑ |
| myself | PEP_STATUS | Identity⇕ |
| update_dentity | PEP_STATUS | Identity⇕ |
| trust_personal_key | PEP_STATUS | Identity |
| key_mistrusted | PEP_STATUS | Identity |
| key_reset_trust | PEP_STATUS | Identity |
| least_trust | PEP_STATUS | String, PEP_comm_type⇑ |
| get_key_rating | PEP_STATUS | String, PEP_comm_type⇑ |
| renew_key | PEP_STATUS | String, Timestamp |
| revoke | PEP_STATUS | String, String |
| key_expired | PEP_STATUS | String, Integer, Bool⇑ |


#### from blacklist.h & OpenPGP_compat.h ####
| Function name | Return Type | Parameters |
|---------------|-------------|------------|
| blacklist_add | PEP_STATUS | String |
| blacklist_delete | PEP_STATUS | String |
| blacklist_is_listed | PEP_STATUS | String, Bool⇑ |
| blacklist_retrieve | PEP_STATUS | StringList⇑ |
| OpenPGP_list_keyinfo | PEP_STATUS | String, StringPairList⇑ |


#### Event Listener & Results ####
| Function name | Return Type | Parameters |
|---------------|-------------|------------|
| registerEventListener | PEP_STATUS | String, Integer, String |
| unregisterEventListener | PEP_STATUS | String, Integer, String |
| deliverHandshakeResult | PEP_STATUS | Identity, PEP_sync_handshake_result |


#### Other ####
| Function name | Return Type | Parameters |
|---------------|-------------|------------|
| version | String |  |
| apiVersion | Integer |  |
| getGpgEnvironment | GpgEnvironment |  |
