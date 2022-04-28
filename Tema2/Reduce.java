import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/* Task-ul de tip Reduce */
public class Reduce implements Comparable<Reduce>{
    public final File file;
    public List<String> longests = new ArrayList<>();
    public List<Map<Integer, Integer>> lengths = new ArrayList<>();
    public float rang;
    public int max;
    public int count_max;

    public Reduce(File file, Map<Integer, Integer> lengths) {
        this.file = file;
        this.lengths.add(lengths);
    }
    // adauga cele mai lungi cuvinte din fiecare Task
    public void append_word(String longests) {
        this.longests.add(longests);
    }
    // adauga lungimile cuvintelor
    public void append_lengths(Map<Integer, Integer> lengths) {
        this.lengths.add(lengths);
    }

    @Override
    public int compareTo(Reduce o) {
        if (o.rang > this.rang)
            return 1;
        else
            return -1;
    }
}
