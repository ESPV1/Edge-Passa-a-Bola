import aedes from "aedes";
import { createServer } from "aedes-server-factory";
import mqtt from "mqtt";

// === Configurações ===
const LOCAL_WS_PORT = 9001;                      // Navegador conecta aqui (WebSocket)
const REMOTE_BROKER = "mqtt://54.172.140.81:1883"; // Broker remoto (TCP normal)
const RECONNECT_MS = 5000;                       // Tempo para tentar reconexão

// === Cria broker local com suporte a WebSocket ===
const broker = aedes();
const server = createServer(broker, { ws: true });

server.listen(LOCAL_WS_PORT, () => {
  console.log(`✅ Broker local com WebSocket ativo em ws://localhost:${LOCAL_WS_PORT}`);
});

// === Conexão e reconexão com broker remoto ===
let bridge;
const bridgeTopicSet = new Set(); // controla mensagens para evitar loop

function connectBridge() {
  console.log("🔗 Conectando ao broker remoto...");

  bridge = mqtt.connect(REMOTE_BROKER);

  bridge.on("connect", () => {
    console.log("✅ Conectado ao broker remoto!");
    bridge.subscribe("#", (err) => {
      if (!err) console.log("📡 Subscrito em todos os tópicos remotos (#)");
    });
  });

  bridge.on("error", (err) => {
    console.error("❌ Erro no broker remoto:", err.message);
  });

  bridge.on("close", () => {
    console.warn("⚠️ Broker remoto desconectado. Tentando reconectar...");
    setTimeout(connectBridge, RECONNECT_MS);
  });

  // 🔄 Quando o remoto publica algo → envia ao local
  bridge.on("message", (topic, payload) => {
    if (!topic.startsWith("$SYS")) {
      const id = topic + payload.toString();
      bridgeTopicSet.add(id); // marca como vindo do remoto
      broker.publish({ topic, payload });
      console.log(`⬇️ [Remoto → Local] ${topic}: ${payload}`);
    }
  });
}

connectBridge();

// 🔼 Quando o local publica algo → envia ao remoto (sem loop)
broker.on("publish", (packet, client) => {
  if (!bridge || !bridge.connected || packet.topic.startsWith("$SYS")) return;

  const id = packet.topic + packet.payload.toString();

  // Evita retransmitir o que veio do remoto
  if (bridgeTopicSet.has(id)) {
    bridgeTopicSet.delete(id);
    return;
  }

  bridge.publish(packet.topic, packet.payload);
  console.log(`⬆️ [Local → Remoto] ${packet.topic}: ${packet.payload}`);
});

// 👥 Logs básicos de conexão de clientes
broker.on("client", (client) => {
  console.log(`🧩 Cliente conectado: ${client ? client.id : "desconhecido"}`);
});

broker.on("clientDisconnect", (client) => {
  console.log(`❎ Cliente desconectado: ${client ? client.id : "desconhecido"}`);
});