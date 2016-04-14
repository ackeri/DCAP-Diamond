package edu.washington.cs.diamond;

import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import java.io.*;
import java.nio.charset.Charset;
import java.util.*;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.http.NameValuePair;
import org.apache.http.client.utils.URLEncodedUtils;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.AbstractHandler;

import redis.clients.jedis.*;

class Utils {
	public static Map<String, String> getQueryParams(String queryString) {
		Map<String, String> result = new HashMap<String, String>();
		String[] paramSplit = queryString.split("&");
		for (String param : paramSplit) {
			String[] keyValueSplit = param.split("=");
			if (keyValueSplit.length == 2) {
				String key = keyValueSplit[0];
				String value = keyValueSplit[1];
				result.put(key, value);
			}
		}
		return result;
	}

	public static Map<String, String> getBodyParams(InputStream requestBody) {
		try {
			byte[] bodyArray = new byte[requestBody.available()];
			requestBody.read(bodyArray);
			String bodyString = new String(bodyArray, "UTF-8");
			List<NameValuePair> bodyParams = URLEncodedUtils.parse(bodyString, Charset.forName("UTF-8"));
			Map<String, String> result = new HashMap<String, String>();
			for (NameValuePair pair : bodyParams) {
				result.put(pair.getName(), pair.getValue());
			}
			return result;
		}
		catch (IOException e) {
			return null;
		}
	}
}

public class KeyValueServer {
	Jedis jedis;
	int numSlaves;
	int numFailures;

	class PutGetHandler extends AbstractHandler {

		@Override
		public void handle(String target, Request baseRequest, HttpServletRequest request, HttpServletResponse response)
				throws IOException, ServletException {

			JsonObject responseJson = new JsonObject();
			int responseCode = HttpServletResponse.SC_BAD_REQUEST;

			if (target.equals("/put")) {
				Map<String, String> bodyParams = Utils.getBodyParams(request.getInputStream());
				String key = bodyParams.get("key");
				String value = bodyParams.get("value");

				if (key != null && value != null) {
					jedis.set(key, value);
					jedis.waitReplicas(numSlaves - numFailures, 10000);
					responseJson.add("key", new JsonPrimitive(key));
					responseJson.add("value", new JsonPrimitive(value));
					responseCode = HttpServletResponse.SC_OK;
				}
			}
			else if (target.equals("/get")) {
				String queryString = request.getQueryString();
				if (queryString == null) {
					queryString = "";
				}
				Map<String, String> queryParams = Utils.getQueryParams(queryString);
				String key = queryParams.get("key");
				String value = null;

				if (key != null) {
					value = jedis.get(key);
					if (value == null) {
						value = "";
					}
					responseJson.add("key", new JsonPrimitive(key));
					responseJson.add("value", new JsonPrimitive(value));
					responseCode = HttpServletResponse.SC_OK;
				}
			}

			response.setStatus(responseCode);
			PrintWriter out = response.getWriter();
			out.print(responseJson.toString());

			baseRequest.setHandled(true);
		}
	}

	private void loadKeys(String keyFile, int numKeys) {
		try {
			BufferedReader reader = new BufferedReader(new FileReader(keyFile));
			String line = reader.readLine();
			int keyNum = 0;
			while (line != null && keyNum < numKeys) {
				jedis.set(line, "temp");
				keyNum++;
				line = reader.readLine();
			}
			reader.close();
		}
		catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void start(int port, String redisHostname, int redisPort, int numSlaves, int numFailures, String keyFile, int numKeys) {
		this.numSlaves = numSlaves;
		this.numFailures = numFailures;
		
		JedisPool pool = new JedisPool(new JedisPoolConfig(), redisHostname, redisPort);
		jedis = null;
		Server server = null;

		try {
			jedis = pool.getResource();

			loadKeys(keyFile, numKeys);

			server = new Server(port);
			server.setHandler(new PutGetHandler());
			server.start();
			server.join();
		}
		catch (Exception e) {
			e.printStackTrace();
			System.exit(1);
		}
		finally {
			if (jedis != null) {
				jedis.close();
			}
		}
		pool.destroy();
	}

	public static void main(String[] args) {
		if (args.length < 7) {
			System.err.println("usage: java Server port redis-hostname redis-port num-slaves num-failures key-file num-keys");
			System.exit(1);
		}
		new KeyValueServer().start(Integer.parseInt(args[0]), args[1], Integer.parseInt(args[2]), Integer.parseInt(args[3]), Integer.parseInt(args[4]), args[5], Integer.parseInt(args[6]));
	}
}