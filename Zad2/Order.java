// author: @ffffffffffffffff0

import java.io.Console;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.HashSet;

class Order implements OrderInterface {
    private ResultListener resultListener;
    private AtomicInteger order_id = new AtomicInteger(0);
    private HashSet<Location> visited = new HashSet<Location>();
    private int H = 101, W = 101;

    Order() {
        loadMazeData();
    }

    private void loadMazeData() {
        try {
            List<String> lines = Files.readAllLines(Paths.get("/home/jonasz/UJ_PRiR/Zad2/maze.data"));
            H = lines.size();
            W = lines.get(0).length();
            maze_data = new char[H][W];
            for (int i = 0; i < H; i++) {
                maze_data[i] = lines.get(i).toCharArray();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private char[][] maze_data = {

    };

    private char reverse(Location l) {
        int r = l.row();
        int c = l.col();
        if (r < 0 || r == H || c < 0 || c == W)
            return '#';
        return maze_data[H - r - 1][c];
    }

    private List<Direction> possibleMoves(Location currentLocation) {
        List<Direction> dir = new ArrayList<Direction>();
        if (reverse(Direction.NORTH.step((currentLocation))) != '#')
            dir.add(Direction.NORTH);
        if (reverse(Direction.SOUTH.step((currentLocation))) != '#')
            dir.add(Direction.SOUTH);
        if (reverse(Direction.WEST.step((currentLocation))) != '#')
            dir.add(Direction.WEST);
        if (reverse(Direction.EAST.step((currentLocation))) != '#')
            dir.add(Direction.EAST);
        return dir;
    }

    private LocationType loc(Location currentLocation) {
        if (reverse(currentLocation) == '#')
            return LocationType.WALL;
        if (reverse(currentLocation) == ' ')
            return LocationType.PASSAGE;
        else
            return LocationType.EXIT;
    }

    private void runThread(int o, Location l) {
        (new Thread() {
            public void run() {
                long wait_time = (long) (Math.random() * 50);
                try {
                    Thread.sleep(wait_time);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                resultListener.result(new ResultImplementation(o, loc(l), possibleMoves(l)));
            }
        }).start();
    }

    public void setResultListener(ResultListener listener) {
        this.resultListener = listener;
    }

    public int order(Location location) {
        if (visited.contains(location)) {
            System.err.println("Location " + location + " already visited");
            System.exit(1);
        }
        if (reverse(location) == '#') {
            System.err.println("Location " + location + " is a wall");
            System.exit(1);
        }
        visited.add(location);
        runThread(order_id.incrementAndGet(), location);
        return order_id.get();
    }
}

class ResultImplementation implements Result {
    private int order_id;
    private LocationType locationType;
    private List<Direction> allowDirections;

    public ResultImplementation(int order_id, LocationType locationType, List<Direction> allowDirections) {
        this.order_id = order_id;
        this.locationType = locationType;
        this.allowDirections = allowDirections;
    }

    public int orderID() {
        return this.order_id;
    }

    public LocationType type() {
        return this.locationType;
    }

    public List<Direction> allowedDirections() {
        return this.allowDirections;
    }
}