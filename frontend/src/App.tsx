import { useEffect, useState } from 'react'
import './App.css'
import mqtt from "mqtt"

function App() {

  const [client, setClient] = useState<any>(null);
  const [isConnected, setIsConnected] = useState(false);

  useEffect(() => {
    const mqttClient = mqtt.connect("ws://localhost:9001");
    setClient(mqttClient);

    mqttClient.on("connect", () => {
      console.log("Conectado ao broker MQTT");
      setIsConnected(true);
    });

    mqttClient.on("error", (err) => {
      console.error("Erro de conexão MQTT:", err);
      mqttClient.end();
    });

    return () => {
      mqttClient.end();
    };
  }, []);

  const handlePublish = (bool: boolean) => {
    if (client && isConnected) {
      const topic = "/TEF/device001/cmd";
      const message = `device001@${bool ? 'on' : 'off'}|`;
      client.publish(topic, message);
      console.log(`Mensagem publicada em ${topic}: ${message}`);
    }
  };

  return (
    <div className='flex gap-5'>
      <button className='bg-red-600 hover:bg-red-700 transition-colors' onClick={() => handlePublish(true)}>Ligar LED</button>
      <button className='bg-green-600 hover:bg-green-700 transition-colors' onClick={() => handlePublish(false)}>Desligar LED</button>
    </div>
  )
}

export default App