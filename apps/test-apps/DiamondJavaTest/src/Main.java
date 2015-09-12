import edu.washington.cs.diamond.Diamond.DStringList;
import edu.washington.cs.diamond.Diamond.DString;

public class Main {
	public static void main(String[] args) {
		threadTest();
	}
	
	/**
	 * Simple test of Diamond syncing
	 */
	public static void basicTest() {
		DString testString1 = new DString("Diamond syncing not working", "javatest:str");
		DString testString2 = new DString("Diamond syncing not working", "javatest:str");
		
		testString1.Lock();
		testString1.Set("Diamond syncing is working");
		testString1.Broadcast();
		testString1.Unlock();
		
		System.out.println(testString2.Value());
	}
	
	/**
	 * Multithreaded ping-pong test
	 */
	public static void threadTest() {
		final DString pingPong = new DString("ping", "javatest:pingpong");
		new Thread(new Runnable() {
			public void run() {
				while (true) {
					System.out.println("Ping start");
					pingPong.Lock();
					System.out.println("Ping locked");
					while (pingPong.Value().equals("ping")) {
						System.out.println("Ping wait");
						pingPong.Wait();
					}
					pingPong.Set("ping");
					pingPong.Signal();
					System.out.println("Ping signaled");
					pingPong.Unlock();
					System.out.println("Ping unlocked");
					System.out.println("ping");
				}
			}
		}).start();
		new Thread(new Runnable() {
			public void run() {
				while (true) {
					System.out.println("Pong start");
					pingPong.Lock();
					System.out.println("Pong locked");
					while (pingPong.Value().equals("pong")) {
						System.out.println("Pong wait");
						pingPong.Wait();
					}
					pingPong.Set("pong");
					pingPong.Signal();
					System.out.println("Pong signaled");
					pingPong.Unlock();
					System.out.println("Pong unlocked");
					System.out.println("pong");
				}
			}
		}).start();
	}
	
	/**
	 * Tests bug that appeared in DiMessage when the chat log grew beyond a certain size
	 */
	public static void listSizeTest() {
		DStringList testList = new DStringList("javatest:testlist");
		testList.Clear();
		testList.Append("Nexus 7: hello from the Nexus 7");
		testList.Append("Nexus 5: Nexus 5 here");
		testList.Append("Nexus 7: Now to get sync working...");
		testList.Append("Nexus 5: what happened");
		testList.Append("Nexus 7: hello");
		testList.Append("Nexus 5: test");
		testList.Append("Nexus 5: testing locks");
		testList.Append("Nexus 5: testing more");
		testList.Append("Nexus 5: broadcast was causing segfaults");
		testList.Append("Nexus 5: trying broadcast again");
		System.out.println(testList.Value(0));
		System.out.println(testList.Size());
	}
}
