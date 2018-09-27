### Function reference for the p≡p JSON Server Adapter. Version “(38) Frankenberg”, API version 0.15.0 ###
Output parameters are denoted by a  **⇑** , InOut parameters are denoted by a  **⇕**  after the parameter type.

#### Message API ####
| Function name | Return Type | Parameters |
|---------------|-------------|------------|
| MIME_encrypt_message | PEP_STATUS | String, Integer, StringList, String⇑, PEP_enc_format, Integer |
| MIME_encrypt_message_for_self | PEP_STATUS | Identity, String, Integer, StringList, String⇑, PEP_enc_format, Integer |
| MIME_decrypt_message | PEP_STATUS | String, Integer, String⇑, StringList⇑, PEP_rating⇑, Integer⇕, String⇑ |
| startKeySync | Void |  |
| stopKeySync | Void |  |
| startKeyserverLookup | Void |  |
| stopKeyserverLookup | Void |  |
| encrypt_message | PEP_STATUS | Message, StringList, Message⇑, PEP_enc_format, Integer |
| encrypt_message_for_self | PEP_STATUS | Identity, Message, StringList, Message⇑, PEP_enc_format, Integer |
| decrypt_message | PEP_STATUS | Message⇕, Message⇑, StringList⇑, PEP_rating⇑, Integer⇕ |
| outgoing_message_rating | PEP_STATUS | Message, PEP_rating⇑ |
| identity_rating | PEP_STATUS | Identity, PEP_rating⇑ |


#### pEp Engine Core API ####
| Function name | Return Type | Parameters |
|---------------|-------------|------------|
| get_trustwords | PEP_STATUS | Identity, Identity, Language, String⇑, Integer⇑, Bool |
| get_languagelist | PEP_STATUS | String⇑ |
| is_pep_user | PEP_STATUS | Identity, Bool⇑ |
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
| set_own_key | PEP_STATUS | Identity⇕, String |
| undo_last_mistrust | PEP_STATUS |  |
| myself | PEP_STATUS | Identity⇕ |
| update_identity | PEP_STATUS | Identity⇕ |
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
| registerEventListener | Void | String, Integer, String |
| unregisterEventListener | Void | String, Integer, String |
| deliverHandshakeResult | PEP_STATUS | Identity, PEP_sync_handshake_result |


#### Other ####
| Function name | Return Type | Parameters |
|---------------|-------------|------------|
| serverVersion | ServerVersion |  |
| version | String |  |
| getGpgEnvironment | GpgEnvironment |  |
| shutdown | Void |  |

