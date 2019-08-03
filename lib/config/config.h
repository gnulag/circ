// nice macro from https://stackoverflow.com/a/10966395/3474615

#define FOREACH_CONFIG_KEY(T)                                                  \
	T (SERVER)                                                                 \
	T (PORT)                                                                   \
	T (SSL)                                                                    \
	T (NICK)                                                                   \
	T (IDENT)                                                                  \
	T (REALNAME)                                                               \
	T (SASL_ENABLED)                                                           \
	T (AUTH_USER)                                                              \
	T (AUTH_PASS)                                                              \
	T (CHANNEL)

#define GENERATE_ENUM(ENUM) CIRC_##ENUM,
#define GENERATE_STRING(STRING) "CIRC_" #STRING,

enum CONFIG_KEY_ENUM
{
	FOREACH_CONFIG_KEY (GENERATE_ENUM)
};

static const char* CONFIG_KEY_STRING[] = { FOREACH_CONFIG_KEY (
  GENERATE_STRING) };

char*
get_config_value (const char* key);
int
get_config_length ();
void
print_config ();
