import java.util.ArrayList;
import java.util.List;

public class Solver {
    public static void main(String[] args) {
        OrderInterface order = new Order();
        Employer parallelEmployer = new ParallelEmployer();
        Location startLocation = new Location(1, 1);
        List<Direction> startDirections = new ArrayList<Direction>();
        startDirections.add(Direction.NORTH);
        startDirections.add(Direction.EAST);
        parallelEmployer.setOrderInterface(order);
        (new Thread() {
            public void run() {
                Location endLocation = parallelEmployer.findExit(startLocation, startDirections);
                System.out.println("Found exit at " + endLocation);
            }
        }).start();
    }
}