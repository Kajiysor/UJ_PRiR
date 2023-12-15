import java.rmi.RemoteException;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;
import java.rmi.server.UnicastRemoteObject;
import java.util.HashMap;
import java.util.Random;

public class RMIHistogram extends UnicastRemoteObject implements RemoteHistogram, Binder {

    public RMIHistogram() throws RemoteException {
        super();
    }

    public synchronized int createHistogram(int bins) throws RemoteException {
        Random randomID = new Random();
        int histogramID = randomID.nextInt(1000000);
        while (histograms_.containsKey(histogramID)) {
            histogramID = randomID.nextInt(1000000);
        }
        int[] histogram = new int[bins];
        histograms_.put(histogramID, histogram);
        return histogramID;
    }

    public synchronized void addToHistogram(int histogramID, int value) throws RemoteException {
        int[] histogram = histograms_.get(histogramID);
        if (value > histogram.length || value < 0) {
            System.out.println("Value: " + value + " out of range for histogram with ID: " + histogramID);
            return;
        }
        histogram[value]++;
    }

    public synchronized int[] getHistogram(int histogramID) throws RemoteException {
        return histograms_.get(histogramID);
    }

    public void bind(String serviceName) {
        try {
            Registry registry = LocateRegistry.getRegistry();
            registry.rebind(serviceName, new RMIHistogram());
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    private HashMap<Integer, int[]> histograms_ = new HashMap<>();
}
