Required libraries:
sudo apt install libmicrohttpd-dev
sudo apt install libjansson-dev
sudo apt install libcmocka-dev
sudo apt install lcov

Use JQ for prettier printing

Examples:
1. Create note (POST /notes):
    curl -X POST http://127.0.0.1:8080/notes \
        -H "Content-Type: application/json" \
        -d '{
            "title": "Buy groceries",
            "content": "Milk, eggs, bread",
            "tags": ["personal", "shopping"]
        }'

2. Get all notes:
    curl http://127.0.0.1:8080/notes | jq

3. Get a note (GET /notes/{id}):
    curl http://127.0.0.1:8080/notes/{id} | jq

4. Get a list of notes with a specific tag (GET /notes?tag=X):
    curl http://127.0.0.1:8080/notes?tag={tag} | jq

5. Update a note (PUT /notes/{id}):
    curl -X PUT http://127.0.0.1:8080/notes/{id} \
    -H "Content-Type: application/json" \
    -d '{
        "title": "Buy groceries today",
        "content": "Milk, eggs, bread, cheese",
        "tags": ["personal", "shopping", "urgent"]
    }'

6. Delete a note (DELETE /notes/{id}):
    curl -X DELETE http://127.0.0.1:8080/notes/{id}
