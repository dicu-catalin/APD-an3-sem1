import java.io.*;
import java.util.*;
import java.util.concurrent.CyclicBarrier;

import static java.lang.Integer.min;

public class Tema2 {
    public static int frag_len; // modificabil to int in main
    public static int nr_docs;
    public static String[] docs;
    public static final String separators = "[_;:/?~\\.,><`\\[\\]{}()!@#$%^&+'=*\"| \t\r\n-]"; // am scos _
    public static List<Maps> tasks = new ArrayList<>();
    public static Reduce[] reduce_tasks;
    public static int nr_tasks = 0;
    public static int workers;
    public static CyclicBarrier barrier;

    public static void main(String[] args) throws IOException {
        if (args.length < 3) {
            System.err.println("Usage: Tema2 <workers> <in_file> <out_file>");
            return;
        }
        workers = Integer.parseInt(args[0]);
        barrier = new CyclicBarrier(workers);
        Thread[] threads = new Thread[workers];
        File in = new File(args[1]);
        Scanner scanner = new Scanner(in);
        // createTask imparte task-urile in functie de numarul de octeti
        createTask(scanner);
        reduce_tasks = new Reduce[nr_docs];
        // se pornesc threadurile
        for (int i = 0; i < workers; i++) {
            threads[i] = new Thread(new MyThread(i));
            threads[i].start();
        }

        for (int i = 0; i < workers; i++) {
            try {
                threads[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        // creeaza fisierul de iesire si scrie in el
        try {
            File myObj = new File(args[2]);
            myObj.createNewFile();
        } catch (IOException e) {
            System.out.println("An error occurred.");
            e.printStackTrace();
        }
        FileWriter out = new FileWriter(args[2]);
        for (Reduce reduced:reduce_tasks) {
            out.write(reduced.file.getName().split("/")[0] + "," +
                    String.format("%.2f", reduced.rang) + "," + reduced.max + "," + reduced.count_max + "\n");
        }
        out.close();
    }

    private static void createTask(Scanner scanner) throws IOException {
        frag_len = scanner.nextInt();
        nr_docs = scanner.nextInt();
        docs = new String[nr_docs];
        scanner.nextLine();
        // inparte Task-urile strict dupa numarul de octeti
        for (int i = 0; i < nr_docs; i++) {
            docs[i] = scanner.nextLine();
            File f = new File(docs[i]);
            int len = 0;
            int file_len = (int) f.length();
            while (len < file_len) {
                int dim = min(frag_len, file_len - len);
                tasks.add(new Maps(f, len, dim));
                nr_tasks++;
                len += frag_len;
            }
        }
        /* adjust_tasks modifica task-urile astfel incat finalul unui task sa nu se opreasca in
        mijlocul unui cuvant */
        adjust_tasks();
    }

    static void adjust_tasks() throws IOException {
        int adjust = 0;
        for (Maps task:tasks) {
            task.start += adjust;
            task.dim -= adjust;
            adjust = 0;
            InputStream in = new FileInputStream(task.file);
            in.skip(task.start);
            int off = 0;
            byte[] first_frag = new byte[frag_len];
            String first_frag_str;
            byte[] second_frag = new byte[frag_len];
            String second_frag_str;
            int read = in.read(first_frag, off, task.dim);
            /* verifica daca ultima pozitie din task este caracter sau separator. In cazul in care este caracter,
            se va completa task-ul cu caractere din urmatorul task pana in momentul in care gaseste un separator. */
            if (read > 0 && separators.indexOf((char) first_frag[read - 1]) == -1) {
                int read2 = in.read(second_frag, off, frag_len);
                while (separators.indexOf((char) second_frag[adjust]) == -1 && adjust < read2)
                    adjust++;
                task.dim += adjust;
            }
        }
    }
}
