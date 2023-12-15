import java.rmi.RemoteException;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;

public class Main {
    public static void main(String[] args) throws RemoteException {
        try {
            int PORT = 1099;
            Registry registry = LocateRegistry.createRegistry(PORT);
            RMIHistogram histogram = new RMIHistogram();
            histogram.bind("Service");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}