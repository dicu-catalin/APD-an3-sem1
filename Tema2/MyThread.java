import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.*;
import java.util.concurrent.BrokenBarrierException;

public class MyThread implements Runnable {
    private final int id;

    public MyThread(int id) {
        this.id = id;
    }

    @Override
    public void run() {
        // se impart task-urile in mod egal intre workeri
        for (int i = id; i < Tema2.nr_tasks; i = i + Tema2.workers) {
            Maps task = Tema2.tasks.get(i);
            InputStream in = null;
            int off = 0;
            byte[] frag = null;
            // citeste fragmentul pentru task
            try {
                in = new FileInputStream(task.file);
                in.skip(task.start);
                frag = new byte[task.dim];
                in.read(frag, off, task.dim);
            } catch (IOException e) {
                e.printStackTrace();
            }
            String frag_str = new String(frag);
            String[] words = frag_str.split(Tema2.separators);
            List<String> longests_words = new ArrayList<>();
            String max_len = "";
            Map<Integer, Integer> lengths = new HashMap<Integer, Integer>();
            /* se parcurge fiecare cuvant din task, iar lungimea lui este adaugata in Lista. Se verifica
            daca acesta este unul dintre cele mai lungi cuvinte si este adaugat si in aceasta lista */
            for (int j = 0; j < words.length; j++) {
                if (max_len.length() < words[j].length()) {
                    longests_words = new ArrayList<>();
                    longests_words.add(words[j]);
                    max_len = words[j];
                } else if (max_len.length() == words[j].length())
                    longests_words.add(words[j]);
                if (lengths.containsKey(words[j].length())) {
                    int count = lengths.get(words[j].length());
                    lengths.replace(words[j].length(), count + 1);
                } else if (words[j].length() > 0)
                    lengths.put(words[j].length(), 1);
            }
            task.longest = longests_words;
            task.lengths = lengths;
        }
        // se asteapta terminarea tuturor task-urilor de tip Map
        try {
            Tema2.barrier.await();
        } catch (InterruptedException | BrokenBarrierException e) {
            e.printStackTrace();
        }
        // thread-ul cu id 0 creeaza noile task-uri de tip reduce
        if (this.id == 0) {
            Map<File, Integer> file_to_index = new HashMap<>();
            int count = 0;
            for(Maps task:Tema2.tasks) {
                int index;
                if (file_to_index.containsKey(task.file)) {
                    index = file_to_index.get(task.file);
                    for (String longest : task.longest) {
                        Tema2.reduce_tasks[index].append_word(longest);
                    }
                    Tema2.reduce_tasks[index].append_lengths(task.lengths);
                }
                else {
                    file_to_index.put(task.file, count);
                    Tema2.reduce_tasks[count] = new Reduce(task.file,  task.lengths);
                    for (String longest : task.longest) {
                        Tema2.reduce_tasks[count].append_word(longest);
                    }
                    count++;
                }
            }
        }
        try {
            Tema2.barrier.await();
        } catch (InterruptedException | BrokenBarrierException e) {
            e.printStackTrace();
        }

        // Numerele fibonacci deja calculate sunt pastrate intr-o lista
        List<Integer> fibonacci = new ArrayList<>();
        fibonacci.add(1);
        fibonacci.add(2);
        // Se impart task-urile de tip reduce in mod egal
        for (int i = id; i < Tema2.nr_docs; i = i + Tema2.workers) {
            float rang = 0;
            int count = 0;
            // Se parcurge lista de lungimi si se calculeaza rangul unui task
            for (Map<Integer, Integer> entry:Tema2.reduce_tasks[i].lengths) {
                for (Map.Entry<Integer, Integer> len:entry.entrySet()) {
                    if (len.getKey() > fibonacci.size())
                        calculate_fibonacci(fibonacci, len.getKey());
                    rang = rang + fibonacci.get(len.getKey() - 1) * len.getValue();
                    count += len.getValue();
                }
            }
            if (count > 0)
                rang = rang / count;
            Tema2.reduce_tasks[i].rang = rang;
            count = 0;
            int longest = 0;
            // parcurge lista celor mai lungi cuvinte gasite in task-urile de tip map
            for (String word:Tema2.reduce_tasks[i].longests) {
                if (word.length() > longest) {
                    count = 1;
                    longest = word.length();
                } else if (word.length() == longest) {
                    count++;
                }
            }
            Tema2.reduce_tasks[i].max = longest;
            Tema2.reduce_tasks[i].count_max = count;
        }
        try {
            Tema2.barrier.await();
        } catch (InterruptedException | BrokenBarrierException e) {
            e.printStackTrace();
        }
        if (id == 0) {
            Arrays.sort(Tema2.reduce_tasks);
        }
    }

    // calculeaza numerele fibonacii pana la pozitia cautata
    public void calculate_fibonacci(List<Integer> fib, int last) {
        while (fib.size() < last) {
            fib.add(fib.get(fib.size()-1) + fib.get(fib.size()-2));
        }
    }


}
