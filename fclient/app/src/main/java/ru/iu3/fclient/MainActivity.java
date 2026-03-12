package ru.iu3.fclient;


import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;

// Для константы RESULT_OK
import android.app.Activity;

import java.util.Arrays;
import java.util.Locale;

import ru.iu3.fclient.databinding.ActivityMainBinding;
import org.apache.commons.codec.DecoderException;
import org.apache.commons.codec.binary.Hex;

public class MainActivity extends AppCompatActivity  implements TransactionEvents{

    /** Тот же TRD, что передаётся в transaction() — одна точка правды для суммы на главной и в пинпаде */
    private static final String DEFAULT_TRD_HEX = "9F0206000000000100";

    static {
        System.loadLibrary("fclient");
        System.loadLibrary("mbedcrypto");
    }

    private ActivityMainBinding binding; //ссылка на activity_main.xml
    private String pin;
    ActivityResultLauncher<Intent> activityResultLauncher; //объект для запуска другой act и передачи значений

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState); //savedInstanceState - сохраненное стостояние или null если первый запуск

        binding = ActivityMainBinding.inflate(getLayoutInflater()); // все view с id из этого layout. Т.е. читаем xml и получаем некие java объекты(кнопки, текст) для работы в дальнейшем
        setContentView(binding.getRoot()); // устанавливается корневой view этого layout как содержимое экрана


        activityResultLauncher  = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(), //контракт вызова активности для получения рузультата
                new ActivityResultCallback<ActivityResult>() { //контракт вызова активности при возврате из дочерней
                    @Override
                    public void onActivityResult(ActivityResult result) { //вызывается когжда пользователь вернулся с другой акт( напрмиер inpad)
                        if (result.getResultCode() == Activity.RESULT_OK) { //проверка результата акт
                            Intent data = result.getData(); //получение результата, который положила дочерняя акт через setresult
                            // обработка результата
                            //String pin = data.getStringExtra("pin");
                            //Toast.makeText(MainActivity.this, pin, Toast.LENGTH_SHORT).show();
                            pin = data.getStringExtra("pin"); //остаем сроку по ключу pin (пин - введенный в pinpadact)
                            synchronized (MainActivity.this) { // синх по объекту будит поток который ждет enterPin(), так натиыный поток узнает что ppin получен
                                MainActivity.this.notifyAll();
                            }
                        }
                    }
                });

        // Сумма: из intent (при возврате из пинпада) или из того же TRD, что пойдёт в transaction()
        TextView ta = findViewById(R.id.txtAmount); // получение view txtAmount
        String amtStr = getIntent().getStringExtra("amount"); // получение значения
        if (amtStr != null && !amtStr.isEmpty()) { // проверка не пустое ли значение
            try {
                long minor = Long.parseLong(amtStr); // перевод в число
                ta.setText("Сумма: " + formatAmount(minor));
            } catch (NumberFormatException e) {
                ta.setText("Сумма: " + formatAmountFromTrd(DEFAULT_TRD_HEX));
            }
        } else {
            ta.setText("Сумма: " + formatAmountFromTrd(DEFAULT_TRD_HEX));
        }

        int res = initRng(); // вызов нативного метода инициал. ГПСЧ
        byte[] v = randomBytes(10); // тоже натив метод
        //TextView tv = binding.sampleText;
        //tv.setText(stringFromJNI());
        byte[] key = new byte[16];
        byte[] data = "HelloBRO14".getBytes();  // дает байты
        Arrays.fill(key, (byte) 0x01);          // заполняет массив key
// Шифрование
        byte[] encrypted = encrypt(key, data); // натив. метод, передаем ключ и данные для шифр
        logInfo("Encrypted: " + Arrays.toString(encrypted));
// Расшифровка
        byte[] decrypted = decrypt(key, encrypted);
        logInfo("Decrypted: " + new String(decrypted));
// Для TextView можно оставить просто результат
        //tv.setText("Original: " + new String(data) + "\n" +
        //        "Decrypted: " + new String(decrypted));
        //Toast.makeText(this, "Hello", Toast.LENGTH_SHORT).show();


    }

    @Override
    public String enterPin(int ptc, String amount) { //вызывается из нативного кода С
        pin = new String();
        Intent it = new Intent(MainActivity.this, PinpadActivity.class); //intent запуск активности 1) аргумент - контекст(наша меин активность) 2) класс целевой активности
        it.putExtra("ptc", ptc); // кладем данные сколько попыток
        it.putExtra("amount", amount); // кладем данные стоимость
        synchronized (MainActivity.this) {
            activityResultLauncher.launch(it); // запуск
            try { // блок перехвата перрывания
                MainActivity.this.wait(); //тек. поток ждет возврата с пинпада (pin), ожидает notifyAll()
            } catch (Exception ex) {
                //todo: log error
            }
        }
        return pin;
    }
    
    public void onButtonClick(View v)
    {
        new Thread(()-> {
            try {
                byte[] trd = stringToHex(DEFAULT_TRD_HEX);
                transaction(trd); //запуск нативного метода
            } catch (Exception ex) {
                // todo: log error
            }
        }).start();
        // Pinpad запускается из enterPin(), который вызывает нативный transaction()
    }
    @Override
    public void transactionResult(boolean result) { // вызывается из нативного кода C
        runOnUiThread(()-> { //выполняет код в UI -потоке Нужно, потому что вызов приходит из нативного потока, а Toast показывать можно только из UI.
            Toast.makeText(MainActivity.this, result ? "ok" : "failed", Toast.LENGTH_SHORT).show(); //вывод сообщения
        });
    }

    public native String stringFromJNI();
    public static native int initRng();
    public static native byte[] randomBytes(int no);

    public native void logInfo(String message);
    public static native byte[] encrypt(byte[] key, byte[] data);
    public static native byte[] decrypt(byte[] key, byte[] data);
    public native boolean transaction(byte[] trd);
    public static byte[] stringToHex(String s)
    {
        byte[] hex;
        try
        {
            hex = Hex.decodeHex(s.toCharArray());
        }
        catch (DecoderException ex)
        {
            hex = null;
        }
        return hex;
    }

    /** Сумма из TRD (6 байт BCD с 4-го по 9-й в hex). Совпадает с тем, что показывается в пинпаде. */
    private static String formatAmountFromTrd(String trdHex) {
        byte[] trd = stringToHex(trdHex);
        if (trd == null || trd.length < 9) return "—";
        // байты 3..8 — сумма в BCD (каждый байт = 2 десятичные цифры), значение в рублях
        long amount = 0;
        for (int i = 3; i < 9; i++) {
            int b = trd[i] & 0xFF;
            amount = amount * 100 + (b >> 4) * 10 + (b & 0x0F);
        }
        return formatAmount(amount);
    }

    /** amount = сумма в рублях (100 → "100.00") — всегда два знака после запятой */
    private static String formatAmount(long amount) {
        return String.format(Locale.ROOT, "%.2f", (double) amount);
    }

}
