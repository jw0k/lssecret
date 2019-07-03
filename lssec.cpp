#include <cstdio>
#include <cstring>
#include <iostream>
#include <libsecret-1/libsecret/secret.h>

const char *USAGE = R"([flags ...]
	-h, --help     Print this message and exit.
	-s, --secrets  Show secret values. Without this flag, all secret values
	               will appear as '***'.
)";

bool show_secrets = false;

void show();
void show_service(SecretService *srvc);
void show_collection(gpointer col_p, gpointer _user_data);
void show_item(gpointer item_p, gpointer _user_data);
void show_secret(SecretItem *item);
void show_attrib(gpointer key, gpointer val, gpointer _user_data);

int main(int argc, char *argv[]) {
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			if (std::strcmp(argv[i], "-s") == 0
			    || std::strcmp(argv[i], "--secrets") == 0)
			{
				show_secrets = true;
			} else if (std::strcmp(argv[i], "-h") == 0
			           || std::strcmp(argv[i], "--help") == 0)
			{
				std::cerr << "usage: " << argv[0] << ' ' << USAGE;
				std::exit(0);
			} else {
				std::cerr << "usage: " << argv[0] << ' ' << USAGE;
				std::exit(1);
			}
		}
	}

	show();
	return 0;
}

void show() {
	GError *err = NULL;
	SecretService *srvc = secret_service_get_sync(
		SECRET_SERVICE_LOAD_COLLECTIONS,
		NULL,
		&err);
	if (err != NULL || srvc == NULL) {
		std::cerr << "Couldn't get secret service\n";
		std::exit(1);
	}
	show_service(srvc);
	g_object_unref(srvc);
}

void show_service(SecretService *srvc) {
	GList *cols = secret_service_get_collections(srvc);
	g_list_foreach(cols, show_collection, NULL);
	g_list_free(cols);
}

void show_collection(gpointer col_p, gpointer _user_data) {
	SecretCollection *col = static_cast<SecretCollection *>(col_p);

	gchar *label = secret_collection_get_label(col);
	std::cout << "Collection:\t" << label << "\n\n";

	GList *items = secret_collection_get_items(col);
	g_list_foreach(items, show_item, NULL);

	g_free(label);
	g_list_free(items);
	g_object_unref(col);
}

void show_item(gpointer item_p, gpointer _user_data) {
	SecretItem *item = static_cast<SecretItem *>(item_p);

	gchar *label = secret_item_get_label(item);
	std::cout << "Item:\t" << label << '\n';

	if (show_secrets) show_secret(item);

	GHashTable *attribs = secret_item_get_attributes(item);
	g_hash_table_foreach(attribs, show_attrib, NULL);

	std::cout << '\n';

	g_hash_table_unref(attribs);
	g_free(label);
	g_object_unref(item);
}

void show_secret(SecretItem *item) {
	GError *err = NULL;
	gboolean success = secret_item_load_secret_sync(item, NULL, &err);
	SecretValue *val = secret_item_get_secret(item);

	if (!success || val == NULL) {
		std::cerr << "Couldn't load secret value. It may not be unlocked.\n";
	} else {
		gsize len;
		const gchar *value = secret_value_get(val, &len);
		// value is not nessesarily null terminated
		std::printf("Value:\t%.*s\n", len, value);
		secret_value_unref(val);
	}
}

void show_attrib(gpointer key_p, gpointer val_p, gpointer _user_data) {
	gchar *key = static_cast<gchar *>(key_p);
	gchar *val = static_cast<gchar *>(val_p);

	std::cout << "Key:\t" << key << "\nValue:\t" << val << '\n';
}