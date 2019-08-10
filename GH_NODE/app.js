const Express = require('express');
const WebSocket = require('ws').Server;
const MongoClient = require("mongodb").MongoClient;

const port = process.env.PORT || 8000;

//const mongoClient = new MongoClient(process.env.MONGODB_URL, { useNewUrlParser: true });

var hist = "e";

const app = Express()
	.get('/', (req, res) => res.send(hist))
	.listen(port, () => console.log(`Listening on ${port}`));

const wss = new WebSocket({ server: app });

wss.on('connection', (ws) => {
	console.log('Client connected');
	ws.on('message', (message) => {
		hist += message + "\n";
		console.log(message);
	});
	ws.on('close', () => console.log('Client disconnected'));
});

