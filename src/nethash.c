#include <assert.h>
#include <curl/curl.h>
#include "flag.h"
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include "nethash.h"
#include <openssl/evp.h>
#include "sha256.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG(format, msg) \
  if (flag__verbose){    \
    printf(format, msg); \
    printf("\n");        \
  }

#define LOG_ERROR(msg)           \
  if (flag__verbose){            \
    fprintf(stderr, "Error:\n"); \
    fprintf(stderr, msg);        \
    fprintf(stderr, "\n");       \
  }

enum constants {
  CID_SIZE = 8,
  CHUNKS_SIZE = SHA256_SIZE / sizeof(uint16_t),
  MEMORY_LIMIT = 2000,
  MAX_MAIN_PAGE_CHARACTERS = 20000,
  MAX_MAIN_PAGE_LINKS = 235,
  MAX_PAGE_LINKS = 2000,
  CURL_TIMEOUT = 10L,
};

struct nethash_ipfs_response {
  char *memory;
  size_t size;
  int error;
};

struct nethash_ipfs_data {
  int two_bytes_count;
  int href_count;
  uint16_t* two_bytes_data;
  char* href_data;
  char get_href;
};

static int nethash_clean_href(const char* src, char* dst){
  if (strstr(src, "http://") != NULL ||
      strstr(src, "https://") != NULL ||
      strstr(src, "/ipfs/") != NULL ||
      strstr(src, "#") != NULL) {
    return -1;
  }
  else if (strstr(src, "../A/") != NULL) {
    strcpy(dst, src + 5);
  }
  else {
    strcpy(dst, src);
  }
  return 0;
}

static void nethash_create_url(char* url, const char* cid, const char* href){
#define DOMAIN "https://ipfs.io/"
  if (href == NULL)
    sprintf(url, DOMAIN "ipfs/%s/wiki/", cid);
  else
    sprintf(url, DOMAIN "ipfs/%s/wiki/%s", cid, href);
#undef DOMAIN
}

static void nethash_extract_two_bytes(const char *html_content, struct nethash_ipfs_data* query_data){
  uint64_t data_size = strlen(html_content);
  int byte_start = query_data->two_bytes_count;
  if (byte_start > data_size)
    byte_start = data_size - sizeof(uint16_t);
  *(query_data->two_bytes_data) = ((unsigned char)html_content[byte_start] << 8) | (unsigned char)html_content[byte_start + 1];
}

static int nethash_extract_href(const char *html_content, struct nethash_ipfs_data* query_data) {
  htmlDocPtr doc = htmlReadMemory(html_content, strlen(html_content), NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
  if (doc == NULL) {
    LOG_ERROR("Could not parse HTML content");
    return -1;
  }

  xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
  if (xpathCtx == NULL) {
    LOG_ERROR("Could not create XPath context");
    xmlFreeDoc(doc);
    return -1;
  }

  xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar *)"//a/@href", xpathCtx);
  if (xpathObj == NULL) {
    LOG_ERROR("Could not evaluate XPath expression");
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    return -1;
  }

  xmlNodeSetPtr nodes = xpathObj->nodesetval;
  int valid_href_count = 0;
  if (nodes) {
    for (int i = 0; i < nodes->nodeNr; i++) {
      xmlNodePtr node = nodes->nodeTab[i];
      if (node->type == XML_ATTRIBUTE_NODE) {
	xmlChar *href = xmlNodeListGetString(doc, node->children, 1);
	char dst[strlen((char*)href) + 1];
	int r = nethash_clean_href((char*)href, dst);
	if (r != -1){
	  valid_href_count++;
	  memcpy(query_data->href_data, dst, strlen((char*)href) + 1);
	}
	xmlFree(href);
	if (valid_href_count >= query_data->href_count) break;
      }
    }
  }
  if (!valid_href_count){
    // No href was found.
    query_data->href_data = NULL;
  }
  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx);
  xmlFreeDoc(doc);

  return 0;
}

static size_t nethash_write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct nethash_ipfs_response *mem = (struct nethash_ipfs_response *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (ptr == NULL) {
    LOG_ERROR("Not enough memory");
    mem->error = 1;
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static int nethash_query_ipfs(struct nethash_ipfs_data* query_data) {
  CURL *curl;
  CURLcode res;
  struct nethash_ipfs_response chunk;

  chunk.memory = malloc(1);
  chunk.size = 0;
  chunk.error = 0;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, query_data->href_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nethash_write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    // Exclude HTTP headers, only get the HTML body.
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    // Set a connection timeout.
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CURL_TIMEOUT);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK || chunk.error) {
      LOG_ERROR("curl_easy_perform() failed");
      return -1;
    } else {
      nethash_extract_two_bytes(chunk.memory, query_data);
      if (query_data->get_href)
	nethash_extract_href(chunk.memory, query_data);
    }

    free(chunk.memory);
    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();

  return 0;
}

int nethash_compute(const char* string, unsigned char* nethash_buffer) {

  unsigned char nethash_raw_buffer[NETHASH_RAW_SIZE] = {0};

  // sha256
  unsigned char sha256_buffer[EVP_MAX_MD_SIZE];
  unsigned int sha256_length;
  sha256_compute(string, sha256_buffer, &sha256_length);
  if (flag__verbose){
    printf("SHA256 Hash: ");
    sha256_print_hash(sha256_buffer, sha256_length);
  }
  if (flag__assert)
    assert(sha256_length == SHA256_SIZE);

  // Create 16 chunks of the sha256 hash, each chunk is 2 bytes.
  // First chunk is used to select a cid and visit the page.
  // Remaining chunks will be used to select a href link and 2 bytes of data from a page.
  // chunks[0] = used to select a cid and visit the page.
  // chunks[1] = used to select 2 bytes of data from the main wiki page.
  // chunks[2] = used to select a new link from the main wiki page and visit it.
  // chunks[3] = used to select 2 bytes of data from this page.
  // chunks[4] = used to select a new link from this page.
  // ...
  // ...
  // chunks[14] = used to select a new link from this page.
  // chunks[15] = used to select 2 bytes of data from this page.
  if (flag__assert)
    assert(CHUNKS_SIZE == 16);
  uint16_t chunks[CHUNKS_SIZE] = {0};
  for (int i = 0, j = 0; i < sha256_length; i += 2, j++){
    chunks[j] = (sha256_buffer[i] << 8) | sha256_buffer[i + 1];
    LOG("Data chunk: 0x%x", chunks[j]);
  }

  // ipfs
  // https://github.com/ipfs/distributed-wikipedia-mirror/blob/main/snapshot-hashes.yml
  const char* cids[CID_SIZE] = {
    "bafybeiaysi4s6lnjev27ln5icwm6tueaw2vdykrtjkwiphwekaywqhcjze",
    "bafybeieuutdavvf55sh3jktq2dpi2hkle6dtmebe7uklod3ramihyf3xa4",
    "bafybeib66xujztkiq7lqbupfz6arzhlncwagva35dx54nj7ipyoqpyozhy",
    "bafybeih4a6ylafdki6ailjrdvmr7o4fbbeceeeuty4v3qyyouiz5koqlpi",
    "bafybeiazgazbrj6qprr4y5hx277u4g2r5nzgo3jnxkhqx56doxdqrzms6y",
    "bafybeibiqlrnmws6psog7rl5ofeci3ontraitllw6wyyswnhxbwdkmw4ka",
    "bafybeiezqkklnjkqywshh4lg65xblaz2scbbdgzip4vkbrc4gn37horokq",
    "bafybeicpnshmz7lhp5vcowscty4v4br33cjv22nhhqestavb2mww6zbswm"
  };

  struct nethash_ipfs_data query_data;
  char* memory = malloc(MEMORY_LIMIT);

  // Moderate cid to the fixed cid size.
  const char* cid = cids[chunks[0] % CID_SIZE];

  // Initialize two_bytes_data, can't be 0 as we use it below.
  uint16_t two_bytes_data = (cid[0] << 8) | cid[1];
  query_data.two_bytes_data = &two_bytes_data;

  // Create the first page url.
  // It will be replaced with a new raw href once the page is visited.
  nethash_create_url(memory, cid, NULL);
  query_data.href_data = memory;
  query_data.get_href = 1;

  // Loop though chunks two at a time.
  for (int i = 1, j=0; i < CHUNKS_SIZE; i += 2, j++){
    // Do not get or process a new href at i = 15.
    if (i >= 15)
      query_data.get_href = 0;

    query_data.two_bytes_count = chunks[i];
    if (query_data.get_href)
      query_data.href_count = chunks[i + 1];

    if (i == 1){
      // Moderate the first round.
      query_data.two_bytes_count %= MAX_MAIN_PAGE_CHARACTERS;
      query_data.href_count %= MAX_MAIN_PAGE_LINKS;
    } else if(query_data.get_href) {
      // Moderate the remaining rounds.
      // We then AND it with two_bytes_data to increase it beyond the limit by chance.
      query_data.href_count %= MAX_PAGE_LINKS;
      query_data.href_count &= two_bytes_data;
    }
    LOG("two_bytes_count: %d", query_data.two_bytes_count);
    LOG("two_bytes_data: %d", two_bytes_data);
    LOG("href_count: %d", query_data.href_count);
    LOG("Visiting url: %s", query_data.href_data);

    if (nethash_query_ipfs(&query_data) != 0)
      return -1;

    // It's possible that no href was found (maybe it was a redirect url and page was empty),
    // in which case go back to the main page.
    if (query_data.get_href){
      if (query_data.href_data == NULL){
	nethash_create_url(memory, cid, NULL);
	query_data.href_data = memory;
	LOG("No href was found, resetting url to: %s", query_data.href_data);
      }
      else {
	LOG("New href: %s", memory);
	char* tmp = malloc(MEMORY_LIMIT);
	memcpy(tmp, memory, MEMORY_LIMIT);
	// Recreate the new url to visit for next round.
	nethash_create_url(memory, cid, tmp);
	free(tmp);
      }
    }

    nethash_raw_buffer[j] = (two_bytes_data >> 8) ^ (two_bytes_data & 0xff);
  }

  // Create the final hash
  memcpy(nethash_buffer, nethash_raw_buffer, NETHASH_RAW_SIZE);
  memcpy(nethash_buffer + NETHASH_RAW_SIZE, sha256_buffer, SHA256_SIZE);

  if (flag__verbose){
    printf("NetHash raw: ");
    for (int i = 0; i < NETHASH_RAW_SIZE; i++)
      printf("%02x", nethash_raw_buffer[i]);
    printf("\n");

    printf("NetHash: ");
    for (int i = 0; i < NETHASH_SIZE; i++)
      printf("%02x", nethash_buffer[i]);
    printf("\n");
  }

  free(memory);
  return 0;
}

