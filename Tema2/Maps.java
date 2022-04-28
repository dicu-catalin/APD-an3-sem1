import java.io.File;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/* Task-ul de tip Map */
public class Maps {
        File file;
        int start;
        int dim;
        // Retine cele mai lungi cuvinte si lungimile cuvintelor
        List<String> longest;
        Map<Integer, Integer> lengths = new HashMap<>();

        public Maps(File file, int start, int dim) {
            this.file = file;
            this.start = start;
            this.dim = dim;
        }

}
