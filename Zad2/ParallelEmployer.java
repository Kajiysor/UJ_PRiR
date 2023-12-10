import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

public class ParallelEmployer implements Employer {
    @Override
    public void setOrderInterface(OrderInterface order) {
        this.orderInterface = order;
    }

    @Override
    public Location findExit(Location startLocation, List<Direction> allowedDirections) {
        orderInterface.setResultListener(new ResultListenerImpl());

        for (Direction direction : allowedDirections) {
            Location location = direction.step(startLocation);
            int orderID = orderInterface.order(location);
            orderIDLocationMap.put(orderID, location);
            exploredLocations.add(location);
        }
        iterateResults();
        return finalLocation;
    }

    synchronized private void iterateResults() {
        if (results.isEmpty()) {
            try {
                this.wait();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        for (Result result : results) {
            if (result.type() == LocationType.EXIT) {
                finalLocation = orderIDLocationMap.get(result.orderID());
                return;
            } else if (result.type() == LocationType.PASSAGE) {
                Location locFromMap = orderIDLocationMap.get(result.orderID());
                for (Direction direction : result.allowedDirections()) {
                    Location location = direction.step(locFromMap);
                    if (!exploredLocations.contains(location)) {
                        int orderID = orderInterface.order(location);
                        orderIDLocationMap.put(orderID, location);
                    }
                    exploredLocations.add(location);
                }
            }
        }
        results.clear();
        iterateResults();
    }

    private class ResultListenerImpl implements ResultListener {
        @Override
        public void result(Result result) {
            synchronized (ParallelEmployer.this) {
                results.add(result);
                ParallelEmployer.this.notify();
            }
        }
    }

    private OrderInterface orderInterface;
    private HashSet<Location> exploredLocations = new HashSet<>();
    private Location finalLocation = null;
    private ArrayList<Result> results = new ArrayList<>();
    private HashMap<Integer, Location> orderIDLocationMap = new HashMap<>();

}