import java.lang.reflect.Array;
import java.rmi.Remote;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Random;
import java.util.ArrayList;
import java.util.stream.IntStream;

class Color {
    public static final String RESET = "\033[0m";
    public static final String RED = "\033[0;31m";
    public static final String GREEN = "\033[0;32m";
}

class RMIHistogramClient {
    RemoteHistogram remoteHistogram;
    int histogramID;

    public int getId() {
        return this.histogramID;
    }

    public RMIHistogramClient(RemoteHistogram remoteHistogram) {
        this.remoteHistogram = remoteHistogram;
        // try {
        // Remote remote = java.rmi.Naming.lookup("//localhost:1099/Service");
        // remoteHistogram = (RemoteHistogram) remote;
        // } catch (Exception e) {
        // e.printStackTrace();
        // }
    }

    synchronized public void createHistogram(int bins) {
        try {
            this.histogramID = remoteHistogram.createHistogram(bins);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void putToHistogram(int value) {
        try {
            remoteHistogram.addToHistogram(this.histogramID, value);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void randomPutRun(int[] values) {
        List<Integer> flatList = new ArrayList<>();
        for (int i = 0; i < values.length; i++) {
            for (int j = 0; j < values[i]; j++) {
                flatList.add(i);
            }
        }
        Collections.shuffle(flatList);
        for (int value : flatList) {
            putToHistogram(value);
        }
    }

    public int[] getHistogram() {
        try {
            return remoteHistogram.getHistogram(this.histogramID);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}

class Tester {
    int histogramHeight;

    Tester(int histogramHeight) {
        this.histogramHeight = histogramHeight;
    }

    HashMap<Integer, int[]> histograms = new HashMap<>();
    Random rand = new Random();

    synchronized public int[] get(int histogramId) {
        return histograms.get(histogramId);
    }

    synchronized public void generateHistogram(int histogramId, int bins) {
        int[] histogram = new int[bins];
        for (int i = 0; i < bins; i++) {
            histogram[i] = rand.nextInt(histogramHeight);
        }
        histograms.put(histogramId, histogram);
    }
}

public class Client {
    synchronized static boolean check(int histogramId, int[] histogram, int[] expectedHistogram) {
        if (histogram.length != expectedHistogram.length) {
            System.err.println("Histogram lengths differ: " + histogram.length + " != " + expectedHistogram.length);
            return false;
        }
        boolean areEqual = IntStream.range(0, histogram.length)
                .allMatch(bin -> histogram[bin] == expectedHistogram[bin]);

        if (!areEqual) {
            System.err.println("----------------------------------------");
            System.err.println("Histograms differ!");
            IntStream.range(0, histogram.length)
                    .forEach(bin -> System.err.println(
                            String.format((areEqual ? Color.GREEN : Color.RED) + "[%d] {%d}: %d ; %d" + Color.RESET,
                                    histogramId, bin, histogram[bin], expectedHistogram[bin])));
            System.err.println("----------------------------------------");

            return false;
        }
        System.out.println(Color.GREEN + String.format("[%d] Histograms are equal!" +
                Color.RESET, histogramId));
        return true;
    }

    public static void main(String[] args) {
        try {
            int clients = args.length > 0 ? Integer.parseInt(args[0]) : 1;
            int bins = args.length > 1 ? Integer.parseInt(args[1]) : 10;
            int histogramHeight = args.length > 2 ? Integer.parseInt(args[2]) : 100;

            Tester tester = new Tester(histogramHeight);

            Remote remote = java.rmi.Naming.lookup("//localhost:1099/Service");
            RemoteHistogram remoteHistogram = (RemoteHistogram) remote;

            List<Thread> threads = new ArrayList<>();
            long startTime = System.currentTimeMillis();
            for (int i = 0; i < clients; i++) {
                Thread thread = new Thread(() -> {
                    RMIHistogramClient RMIClient = new RMIHistogramClient(remoteHistogram);
                    RMIClient.createHistogram(bins);

                    int histogramId = RMIClient.getId();
                    tester.generateHistogram(histogramId, bins);
                    int[] expectedHistogram = tester.get(histogramId);
                    RMIClient.randomPutRun(expectedHistogram.clone());

                    int[] histogram = RMIClient.getHistogram();
                    Client.check(histogramId, histogram, expectedHistogram);
                });
                threads.add(thread);
                thread.start();
            }

            for (Thread thread : threads) {
                try {
                    thread.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            long endTime = System.currentTimeMillis();
            long totalTime = endTime - startTime;
            System.out.println("Clients: " + clients);
            System.out.println("Bins: " + bins);
            System.out.println("Histogram height: " + histogramHeight);
            System.out.println("Total time taken: " + totalTime / 1000.0 + " seconds");
        } catch (Exception e) {
            // TODO: handle exception
        }
    }
}