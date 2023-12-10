import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

public class ParallelEmployer implements Employer {
    @Override
    public void setOrderInterface(OrderInterface order) {
        this.orderInterface_ = order;
    }

    @Override
    public Location findExit(Location startLocation, List<Direction> allowedDirections) {
        orderInterface_.setResultListener(new ResultListenerImpl());

        for (Direction direction : allowedDirections) {
            Location location = direction.step(startLocation);
            orderIDLocationMap_.put(orderInterface_.order(location), location);
            exploredLocations_.add(location);
        }
        try {
            iterateResults();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return finalLocation_;
    }

    synchronized private void iterateResults() throws InterruptedException {
        if (results_.isEmpty()) {
            this.wait();
        }

        for (Result result : results_) {
            if (result.type() == LocationType.EXIT) {
                finalLocation_ = orderIDLocationMap_.get(result.orderID());
                return;
            } else {
                for (Direction direction : result.allowedDirections()) {
                    Location location = direction.step(orderIDLocationMap_.get(result.orderID()));
                    if (!exploredLocations_.contains(location)) {
                        orderIDLocationMap_.put(orderInterface_.order(location), location);
                    }
                    exploredLocations_.add(location);
                }
            }
        }
        results_.clear();
        iterateResults();
    }

    private class ResultListenerImpl implements ResultListener {
        @Override
        public void result(Result result) {
            synchronized (ParallelEmployer.this) {
                results_.add(result);
                ParallelEmployer.this.notify();
            }
        }
    }

    private OrderInterface orderInterface_;
    private HashSet<Location> exploredLocations_ = new HashSet<>();
    private Location finalLocation_ = null;
    private ArrayList<Result> results_ = new ArrayList<>();
    private HashMap<Integer, Location> orderIDLocationMap_ = new HashMap<>();

}
