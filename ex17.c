#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAX_DATA 512
#define MAX_ROWS 100

struct Address {
  int id;
  int set;
  char *name;
  char *email;
};

struct Database {
  int max_data;
  int max_rows;
  struct Address *rows;
};

struct Connection {
  FILE *file;
  struct Database *db;
};

void Database_close(struct Connection *conn);

void die(const char *message, struct Connection *conn)
{
  if (errno) {
    perror(message);
  } else {
    printf("ERROR: %s\n", message);
  }

  if (conn)
    Database_close(conn);

  exit(1);
}

void Address_print(struct Address *address)
{
  printf("%d %s %s\n", address->id, address->name, address->email);
}

void Database_load(struct Connection *conn)
{
  int rc = fread(conn->db, sizeof(struct Database), 1, conn->file);
  if (rc != 1)
    die("Failed to load database.", conn);

  rc = fread(&conn->db->max_data, sizeof(int), 1, conn->file);
  if (rc != 1)
    die("Failed to load max_data info.", conn);

  rc = fread(&conn->db->max_rows, sizeof(int), 1, conn->file);
  if (rc != 1)
    die("Failed to load max_rows info.", conn);

  conn->db->rows = malloc(sizeof(struct Address) * conn->db->max_rows);
  if (!conn->db->rows)
    die("Memory error at Database_load", conn);

  rc = fread(conn->db->rows, sizeof(struct Address) * conn->db->max_rows, 1, conn->file);
  if (rc != 1)
    die("Failed to load database rows.", conn);

  for (int i = 0; i < conn->db->max_rows; i++) {
    conn->db->rows[i].name = malloc(conn->db->max_data);
    rc = fread(&conn->db->rows[i].name, conn->db->max_data, 1, conn->file);
    if (rc != 1)
      die("Failed to load database address name.", conn);

    conn->db->rows[i].email = malloc(conn->db->max_data);
    rc = fread(&conn->db->rows[i].email, conn->db->max_data, 1, conn->file);
    if (rc != 1)
      die("Failed to load database address email.", conn);
  }
}

struct Connection *Database_open(const char *filename, char mode)
{
  struct Connection *conn = malloc(sizeof(struct Connection));
  if (!conn)
    die("Memory error", conn);

  conn->db = malloc(sizeof(struct Database));
  if (!conn->db)
    die("Memory error", NULL);

  if (mode == 'c') {
    conn->file = fopen(filename, "w");
  } else {
    conn->file = fopen(filename, "r+");

    if (conn->file) {
      Database_load(conn);
    }
  }

  if (!conn->file)
    die("Failed to open the file", conn);

  return conn;
}

void Database_close(struct Connection *conn)
{
  if (conn) {
    if (conn->file)
      fclose(conn->file);
    if (conn->db)
      free(conn->db);
    free(conn);
  }
}

void Database_write(struct Connection *conn)
{
  rewind(conn->file);

  int rc = fwrite(conn->db, sizeof(struct Database), 1, conn->file);
  if (rc != 1)
    die("Failed to write to database.", conn);

  rc = fwrite(&conn->db->max_data, sizeof(int), 1, conn->file);
  if (rc != 1)
    die("Failed to write to database.", conn);

  rc = fwrite(&conn->db->max_rows, sizeof(int), 1, conn->file);
  if (rc != 1)
    die("Failed to write to database.", conn);

  rc = fwrite(conn->db->rows, sizeof(struct Address) * conn->db->max_rows, 1, conn->file);
  if (rc != 1)
    die("Failed to write to database.", conn);

  for (int i=0; i < conn->db->max_rows; i++) {
    rc = fwrite(&conn->db->rows[i].name, conn->db->max_data, 1, conn->file);
    if (rc != 1)
      die("Failed to write to database", conn);
    fseek(conn->file, -1, SEEK_CUR);
    char *some_name = "";
    int read = fread(some_name, conn->db->max_data, 1, conn->file);
    if (read == 0)
      die("blablabla", conn);

    rc = fwrite(&conn->db->rows[i].email, conn->db->max_data, 1, conn->file);
    if (rc != 1)
      die("Failed to write to database", conn);
  }

  rc = fflush(conn->file);
  if (rc == -1)
    die("Cannot flush database.", conn);
}

void Database_create(struct Connection *conn, int max_rows, int max_data)
{
  int i = 0;

  conn->db->max_rows = max_rows;
  conn->db->max_data = max_data;
  conn->db->rows = malloc(sizeof(struct Address)*conn->db->max_rows);

  for (i = 0; i < conn->db->max_rows; i++) {
    struct Address addr = {.id = i, .set = 0};
    conn->db->rows[i] = addr;
  }
}

void Database_set(struct Connection *conn, int id, const char *name, const char *email)
{
  struct Address *addr = &conn->db->rows[id];
  if (addr->set)
    die("Already set, delete it first", conn);

  if (addr->name == NULL)
    addr->name = malloc(conn->db->max_data);

  if (addr->email == NULL)
    addr->email = malloc(conn->db->max_data);

  addr->set = 1;
  char *res = "";
  if (strlen(name) >= conn->db->max_data) {
    res = strncpy(addr->name, name, conn->db->max_data);
    res[conn->db->max_data - 1] = '\0';
  } else {
    res = strncpy(addr->name, name, conn->db->max_data);
  }

  if (!res)
    die("Name copy failed", conn);

  if (strlen(email) >= conn->db->max_data) {
    res = strncpy(addr->email, email, conn->db->max_data);
    res[conn->db->max_data - 1] = '\0';
  } else {
    res = strncpy(addr->email, email, conn->db->max_data);
  }

  if (!res)
    die("Email copy failed", conn);
}

void Database_get(struct Connection *conn, int id)
{
  struct Address *addr = &conn->db->rows[id];

  if (addr->set) {
    Address_print(addr);
  } else {
    die("ID is not set", conn);
  }
}

void Database_delete(struct Connection *conn, int id)
{
  struct Address addr = {.id = id, .set = 0};
  conn->db->rows[id] = addr;
}

void Database_list(struct Connection *conn)
{
  int i = 0;
  struct Database *db = conn->db;

  for (i = 0; i < conn->db->max_rows; i++) {
    struct Address *cur = &db->rows[i];

    if (cur->set) {
      Address_print(cur);
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc < 3)
    die("USAGE: ex17 <dbfile> <action> [action params]", NULL);

  char *filename = argv[1];
  char action = argv[2][0];
  int max_data = MAX_DATA;
  int max_rows = MAX_ROWS;
  if (action == 'c') {
    max_rows = atoi(argv[3]);
    max_data = atoi(argv[4]);
  }

  struct Connection *conn = Database_open(filename, action);
  int id = 0;

  if (argc > 3 && action != 'c') id = atoi(argv[3]);
  if (id >= max_rows) die("There's not that many record.", conn);


  switch (action) {
    case 'c':
      Database_create(conn, max_rows, max_data);
      Database_write(conn);
      break;
    
    case 'g':
      if (argc != 4)
        die("Need an id to get", conn);

      Database_get(conn, id);
      break;

    case 's':
      if (argc != 6)
        die("Need id, name, email to set", conn);

      Database_set(conn, id, argv[4], argv[5]);
      Database_write(conn);
      break;

    case 'd':
      if (argc != 4)
        die("Need id to delete", conn);
    
      Database_delete(conn, id);
      Database_write(conn);
      break;

    case 'l':
      Database_list(conn);
      break;
    default:
      die("Invalid action: c=create, g=get, s=set, d=del, l=list", conn);
  }

  Database_close(conn);

  return EXIT_SUCCESS;
}

