#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

// Arduinoのピン設定.
#define SER     2   // シフトレジスタに書き込むための信号を入力するピン.
#define RCLK    3   // データを出力にするときに1.
#define SRCK    4   // 変更を確定するときに1.
#define CLR     5   // データを消すピン.0にするとデータが消える.
#define START   6   // STARTボタン. 1になると始まる.次に1になると止まる.

// イベントの種類.
typedef enum {
    //メタイベント               //FF｜イベントタイプ｜データ長｜データ
    SEQUENCE_NUMBER,          //FF 00 02 シーケンス番号、フォーマット0と1
    COMMENT,                    //FF 01 len data　テキストを書き込むためのイベント
    COPY_RIGHT,                 //FF 02 len data　著作権が書いてある
    SEQUENCE_TRACK_NAME,        //FF 03 len data　シーケンス名(曲名)・トラック名
    INSTRUMENT_NAME,            //FF 04 len data　楽器名
    LYRIC,                      //FF 05 len data　lyricは歌詞の事
    MARKER,                     //FF 06 len data　リハーサル記号やセクション名のような、シーケンスのその時点の名称を記述する
    QUEUE_POINT,                //FF 07 len data　曲データ中、このイベントが挿入されている位置で、曲以外の進行を記述する。(画面のフェードアウトのような動画等に対する進行)
    PROGRAM_NAME,               //FF 08 len data　プログラムチェンジとバンクチェンジで表している音色名を記載する（General MIDIが制定される前の遺物）
    DEVICE_NAME,                //FF 09 len data　このイベントがあるトラックがどのデバイスに配置されるのかということを示す。このイベントは、1トラックにつき1度だけ、最初に呼ばれるべきもの
    CHANNEL_PREFIX,             //FF 20 01 cc MIDIイベントじゃないのに(SysEx or メタイベント)MIDIチャンネルを指定する際に用いる
    PORT_SPECIFIED,             //FF 21 01 pp 出力ポートの指定  0～3がそれぞれポートの1～4に対応する(例外はあるが、データのみでは判別不可)
    END_OF_TRACK,               //FF 2F 00　　トラックの最後であることを表す
    SET_TEMPO,                  //FF 51 03 tt tt tt　tttttt(3byte)には、4分音符の長さをμ秒で表したものを格納する。BPM = 120の時、4分音符の長さは 60 x 10^6 / 120 = 500000[μs]
    TIME_SIGNATURE,             //FF 58 04 nn dd cc bb | 拍子 nn = 分子 dd = 分子 cc = メトロノーム間隔[四分音符間隔なら18H] bb = 四分音符当たりの三十二分音符の数
    KEY_SIGNATURE,              //FF 59 02 sf ml キー(調)を示す   sf = 正:シャープの数 or 負:フラットの数       ml = 0:長調(メジャー) 1:短調(マイナー)
    SPECIAL_META_EVENT,       //FF 7F len data　最初の1バイトはメーカIDで、その後に独自フォーマットのデータが続く
    FF_NONE,                    //何も見つからなかった時、これを入れる

    //システムエクスクルシーブメッセージを格納するイベント。個々の楽器メーカが独自に作成したメッセージを格納する。MIDIデバイスに何かしら設定するのに利用される
    SYSEX,                      //F0 データ長 エクスクルシーブメッセージ F7
    F7_SYSEX,                   //F7 データ長 エクスクルシーブメッセージ

    //MIDIイベント
    POLYPHONIC_KEY_PRESSURE,    //An kk vv nのチャンネルのkk番目のノートにベロシティvvのプレッシャー情報を与える
    CONTROL,                    //Bn cc vv コントローラ設定。チャンネルnでコントローラナンバーccにvvの値を送る
    PROGRAM_CHANGE,             //Cn pp プログラムチェンジ。チャンネルnで、音色をppに変更する
    CHANNEL_PRESSURE,           //Dn vv チャンネルnに対してプレッシャー情報vvを送信する。
    PITCH_BENT,                 //En mm ll ピッチベント。なぜかリトルエンディアンらしい
    NOTE_ON,                    //9n ノート・オン
    NOTE_OFF                    //8n ノート・オフ
} EVENT;

// ドレミ.
// typedef enum {
//     LOW_C = 48,  // １オク下ド
//     LOW_Cs,      // １オク下ド#
//     LOW_D = 50,  // １オク下レ
//     LOW_Ds,      // １オク下レ#
//     LOW_E,       // １オク下ミ
//     LOW_F,       // １オク下ファ
//     LOW_Fs,      // １オク下ファ#
//     LOW_G,       // １オク下ソ
//     LOW_Gs,      // １オク下ソ#
//     LOW_A,       // １オク下ラ
//     LOW_As,      // １オク下ラ#
//     LOW_B,       // １オク下シ
//     NML_C = 60,  // 通常ド
//     NML_Cs,      // 通常ド#
//     NML_D,       // 通常レ
//     NML_Ds,      // 通常レ#
//     NML_E,       // 通常ミ
//     NML_F,       // 通常ファ
//     NML_Fs,      // 通常ファ#
//     NML_G,       // 通常ソ
//     NML_Gs,      // 通常ソ#
//     NML_A,       // 通常ラ
//     NML_As,      // 通常ラ#
//     NML_B,       // 通常シ
//     HI_C = 72,   // １オク上ド
//     HI_Cs,       // １オク上ド#
//     HI_D,        // １オク上レ
//     HI_Ds,       // １オク上レ#
//     HI_E,        // １オク上ミ
//     HI_F,        // １オク上ファ
//     HI_Fs,       // １オク上ファ#
//     HI_G,        // １オク上ソ
//     HI_Gs,       // １オク上ソ#
//     HI_A,        // １オク上ラ
//     HI_As,       // １オク上ラ#
//     HI_B,        // １オク上シ
//     HIHI_C = 84, // ２オク上ド
//     HIHI_Cs,     // ２オク上ド#
//     HIHI_D,      // ２オク上レ
//     HIHI_Ds,     // ２オク上レ#
//     HIHI_E,      // ２オク上ミ
//     HIHI_F,      // ２オク上ファ
//     HIHI_Fs,     // ２オク上ファ#
//     HIHI_G,      // ２オク上ソ
//     HIHI_Gs,     // ２オク上ソ#
//     HIHI_A,      // ２オク上ラ
//     HIHI_As,     // ２オク上ラ#
//     HIHI_B       // ２オク上シ
// } NOTE;

// イベント構造体.
typedef struct {
    EVENT eventtype;                // イベントの種類.
    size_t datasize;            // イベントのデータサイズ.
    const unsigned char* data;  // イベントデータ.
    size_t deltatime;                // デルタタイム.
} Event;

// トラック構造体.
typedef struct {
    Event* event;       // イベント.
    size_t eventnum;    // イベント数.
    size_t tracksize;   // トラックサイズ.
    bool hasNote;       // NoteOn/Offの情報を持っているか.
} Track;

// バイト列をintに変換.
int BytesToInt(unsigned char* bytes, int size) {
    int result = 0;
    for (int i = 0; i < size; i++) {
        result += bytes[i] << (8 * (size - i - 1));
    }
    return result;
}

// NOTE_ON関数出力用.
// FILE*    outfile    : Arduinoプログラム出力先ファイル.
// int      instrument : 出力先楽器ナンバー（Track）.
// u_char*  note       : LOW_C ~ HIHI_B.
void printNoteOn(FILE* outfile, int instrument, unsigned char* note_num) {
    fprintf(outfile, "  note_on(%d, %d);\n", instrument, BytesToInt(note_num, 2));
}

// NOTE_OFF関数出力用.
// FILE*    outfile    : Arduinoプログラム出力先ファイル.
// int      instrument : 出力先楽器ナンバー（Track）.
// u_char[2]  note       : LOW_C ~ HIHI_B.
void printNoteOff(FILE* outfile, int instrument, unsigned char* note) {
    fprintf(outfile, "  note_off(%d, %d);\n", instrument, BytesToInt(note, 2));
}

// delay関数出力用.
void printMicroDelay(FILE* outfile, int microsec) {
    fprintf(outfile, "  delayMicroseconds(%d);\n", microsec);
}

int main() {
    // 変換したいファイル名.
    char file_name[] = "trackmugen.mid";
    // ファイルを開く.
    FILE* midi_file = fopen(file_name, "rb");

    // トラック数.
    int maxTrackNum = 0;

    // hasNote = trueのTrackの数
    int note_track_num = 0; 

    // 分解能.
    int resolution = 0;

    // フォーマット.
    int format = 0;

    if (midi_file == NULL) {
        printf("%sを開けませんでした\n", file_name);
        fclose(midi_file);
        return -1;
    } else {
        printf("%sを開きました\n", file_name);
    }

    // midi_fileのサイズを取得.
    fseek(midi_file, 0, SEEK_END);
    int file_size = ftell(midi_file);
    fseek(midi_file, 0, SEEK_SET);

    // midi_fileの内容を格納する.
    unsigned char* fileData = (unsigned char*)malloc(file_size);

    // ファイルの内容をバッファに読み込む.
    fread(fileData, file_size, 1, midi_file);

    // midi_fileを閉じる.
    fclose(midi_file);
    // ファイルの内容を表示.
    for (int i = 0; i < file_size; i++) {
        printf("%02X ", fileData[i]);
    }
    printf("\n");

    // ファイルヘッダ確認.
    if (fileData[0] == 'M' && fileData[1] == 'T' && fileData[2] == 'h' && fileData[3] == 'd') {
        printf("ファイルヘッダ確認成功\n");
    } else {
        return -1;
    }

    // ヘッダサイズが6であることを確認.
    if (fileData[4] == 0x00 && fileData[5] == 0x00 && fileData[6] == 0x00 && fileData[7] == 0x06) {
        printf("ヘッダサイズ確認成功\n");
    } else {
        return -1;
    }

    // フォーマットを確認.
    // フォーマット0は全てのチャンネルのデータを一つのトラックにまとめたもの.
    // フォーマット1は複数のチャンネルを用いてデータをまとめたもの.
    // フォーマット2はシーケンスパターンをそのまま保存する方式(省略).
    // フォーマットが0の時.
    if (fileData[8] == 0 && fileData[9] == 0) {
        // トラック数は1でないとフォーマット情報と合わない.
        if (fileData[10] != 0 || fileData[11] != 1) {
            return -1;
        }
        maxTrackNum = 1;
        format = 0;
        printf("フォーマット%d\n", format);
    }
    // フォーマットが1の時.
    else if (fileData[8] == 0 && fileData[9] == 1) {
        // トラック数を獲得(2バイト).
        maxTrackNum = (fileData[10] << 8) + fileData[11];
        // maxTrackNumが0だと色々まずいので落とす.
        if (maxTrackNum == 0) {
            return -1;
        }
        format = 1;
        printf("フォーマット%d\n", format);
    }
    // それ以外のフォーマットはエラー.
    else {
        return -1;
    }

    // 時間単位の獲得(分解能のこと).
    // 最初のビットが0ならば、何小節の何拍目のデータといった方式でトラックチャンクが作られる.
    // 最初のビットが1ならば、何分何秒といったデータといった方式で保存する.
    // 大抵は前者.
    if (fileData[12] & 0b10000000) {
        // 何分何秒のやつ(今回は考えないのでスルーする).
        printf("この時間単位は対応していません\n");
        return -1;
    }
    else {
        // 何小節何拍目のやつ.
        resolution = (fileData[12] << 8) + fileData[13];
    }

    // 分解能を四分音符の長さに変換.
    // 例えばresolutionが480の時、四分音符の長さは1/480秒.
    // 1秒 = 1000ミリ秒なので、四分音符の長さは1000 / 480 = 2.0833ミリ秒.
    // これをArduinoのdelayMicroseconds関数に変換すると、delayMicroseconds(2083)になる.

    

    // トラック分のTrackを用意.
    Track track[maxTrackNum];
    int TrackPoint = 0;

    // NoteOn/Offの所持状況をfalseにリセット.
    for (int i = 0; i < maxTrackNum; i++) {
        track[i].hasNote = false;
    }
    
    for (int i = 14; i < file_size; i++) {
        // トラックヘッダ確認.
        printf("%02X ", fileData[i]);
        if (fileData[i] == 'M' && fileData[i + 1] == 'T' && fileData[i + 2] == 'r' && fileData[i + 3] == 'k') {
            printf("初期トラックヘッダ確認成功\n");
            // MTrk分のバイトを進める.
            i += 4;
            // トラックサイズを取得.
            size_t trackSize = BytesToInt(&fileData[i], 4); 
            
            // トラックサイズ分のバイトを進める.
            i += 4;
            // イベントの数をカウントする.
            size_t count = 0;

            // 要素を大体確認する.
            printf("トラック%d解析1週目開始...\n", TrackPoint + 1);
            // 最初はデルタタイムのデータが来る.
            bool delta = true;
            for (int j = i; j < (trackSize + i);) {
                //printf("j: %d\n", j);
                //
                if (delta) {
                    //printf("デルタタイム ");
                    for (int k = 0; k < 4; k++) {
                        if (fileData[j + k] & 0b10000000) {
                            // なにもしない.
                        } else {
                            // デルタタイム分のバイト数進める.
                            j += k + 1;
                            break;
                        }
                    }
                } else { // イベント処理に関する処理.
                    if (fileData[j] == 0xff){
                        // FFから始まるデータはlenバイトでバイトの長さを決める.
                        unsigned char len = fileData[j + 2];
                        j += 3 + len;
                        count++;
                    }
                    else if (fileData[j] == 0xf0 || fileData[j] == 0xf7) {
                        // 0xf0分のバイトを飛ばす.
                        j += 1;
                        int k, len = 0;
                        for (k = 0; k < 4; k++){
                            len <<= 7;
                            len += fileData[j + k] & 0x7f;
                            if (fileData[j + k] & 0x80) {
                                // なにもしない.
                            } else {
                                break;
                            }
                        }
                        j += k + 1;
                        j += len;
                        count++;
                    }
                    // 9n 8n An Bn Enの時は3バイトのデータが来る.
                    else if ((fileData[j] & 0xf0) == 0x90 || (fileData[j] & 0xf0) == 0x80 || (fileData[j] & 0xf0) == 0xA0 || (fileData[j] & 0xf0) == 0xB0 || (fileData[j] & 0xf0) == 0xE0) {
                        // 9n 8nがあるならノートの情報がある.すでにあることがわかっているなら飛ばす.
                        if ((fileData[j] & 0xf0) == 0x90 || (fileData[j] & 0xf0) == 0x80 && !track[TrackPoint].hasNote) {
                            track[TrackPoint].hasNote = true;
                            note_track_num++;
                        }
                        j += 3;
                        count++;
                    }
                    // Cn Dnの時は2バイトのデータが来る.
                    else if ((fileData[j] & 0xf0) == 0xC0 || (fileData[j] & 0xf0) == 0xD0) {
                        j += 2;
                        count++;
                    }
                    else {
                        // それ以外は定義されていない.
                        printf("0x%02X は定義されていません\n", fileData[j]);
                        return -1;
                    }
                }
                delta = !delta;
                //printf("読み取り成功\n");
            }
            printf("トラック%d解析1週目完了\n", TrackPoint + 1);

            // trackに保存.
            track[TrackPoint].tracksize = trackSize;
            track[TrackPoint].eventnum = count;
            Event e[count];
            track[TrackPoint].event = e;
            printf("トラックサイズ: %d\n", trackSize);
            printf("イベント数: %d\n", count);

            // 要素を具体的に格納していく.
            printf("トラック%d解析2週目開始...\n", TrackPoint + 1);
            delta = true;
            int eventPoint = 0;
            for (int j = i; j < (trackSize + i);){
                //printf("j: %d\n", j);
                //printf("%02X ", fileData[j]);
                if (delta) {
                    // デルタタイムのデータを取得.
                    unsigned int delta_time = 0;
                    int k;
                    for (k = 0; k < 4; k++) {
                        delta_time <<= 7;
                        delta_time += fileData[j + k] & 0x7f;
                        if (fileData[j + k] & 0x80) {
                            continue;
                        } else {
                            break;
                        }   
                    }
                    // デルタタイム分のバイト数進める.
                    j += k + 1;
                    track[TrackPoint].event[eventPoint].deltatime = delta_time;
                }
                else {  // イベントとそれに基づくデータの格納処理.
                    // FFの時.
                    if (fileData[j] == 0xff) {
                        // データのイベントタイプを判別する値.
                        unsigned char eventType = fileData[j + 1];
                        // FFから始まるデータはlenバイトでバイトの長さを決める.
                        unsigned char len = fileData[j + 2];
                        // 長さのデータを格納.
                        track[TrackPoint].event[eventPoint].datasize = len;
                        if (len == 0) {
                            // サイズが0の時はデータを格納しない.
                            track[TrackPoint].event[eventPoint].data = NULL;
                        }
                        else {
                            // データの開始地点のポインタを格納.
                            track[TrackPoint].event[eventPoint].data = &fileData[j + 3];
                        }

                        // イベントタイプをそれぞれ格納.
                        switch (eventType)
                        {
                        case 0: track[TrackPoint].event[eventPoint].eventtype = SEQUENCE_NUMBER; break;
                        case 1:track[TrackPoint].event[eventPoint].eventtype = COMMENT; break;
                        case 2: track[TrackPoint].event[eventPoint].eventtype = COPY_RIGHT; break;
                        case 3: track[TrackPoint].event[eventPoint].eventtype = SEQUENCE_TRACK_NAME; break;
                        case 4: track[TrackPoint].event[eventPoint].eventtype = INSTRUMENT_NAME; break;
                        case 5: track[TrackPoint].event[eventPoint].eventtype = LYRIC; break;
                        case 6: track[TrackPoint].event[eventPoint].eventtype = MARKER; break;
                        case 7: track[TrackPoint].event[eventPoint].eventtype = QUEUE_POINT; break;
                        case 8: track[TrackPoint].event[eventPoint].eventtype = PROGRAM_NAME; break;
                        case 9: track[TrackPoint].event[eventPoint].eventtype = DEVICE_NAME; break;
                        case 0x20: track[TrackPoint].event[eventPoint].eventtype = CHANNEL_PREFIX; break;
                        case 0x21: track[TrackPoint].event[eventPoint].eventtype = PORT_SPECIFIED; break;
                        case 0x2f: track[TrackPoint].event[eventPoint].eventtype = END_OF_TRACK; break;
                        case 0x51: track[TrackPoint].event[eventPoint].eventtype = SET_TEMPO; break;
                        case 0x58: track[TrackPoint].event[eventPoint].eventtype = TIME_SIGNATURE; break;
                        case 0x59: track[TrackPoint].event[eventPoint].eventtype = KEY_SIGNATURE; break;
                        case 0x7f: track[TrackPoint].event[eventPoint].eventtype = SPECIAL_META_EVENT; break;


                        default: track[TrackPoint].event[eventPoint].eventtype = FF_NONE; break;
                        }
                        j += 3 + len;
                    }
                    else if (fileData[j] == 0xf0 || fileData[j] == 0xf7) {
                        // 0xf0分のバイトを飛ばす.
                        j += 1;
                        // lenは可変長なのでその分のバイトを取得.
                        int len = 0;
                        for (int k = 0; k < 4; k++)
                        {
                            len <<= 7;
                            len += fileData[j + k] & 0x7f;
                            if (fileData[j + k] & 0x80) { // MSBが1なら次のビットも確認する.
                                // なにもしない.
                            } else {
                                j += k + 1;
                                break;
                            }
                        }

                        // イベントの格納.
                        track[TrackPoint].event[eventPoint].eventtype = SYSEX;
                        track[TrackPoint].event[eventPoint].datasize = len;
                        track[TrackPoint].event[eventPoint].data = &fileData[j];
                        
                        j += len;
                    }
                    // MIDIイベントは,識別子にデータが入っているのでイベント情報が格納されているバイトごとデータを獲得する.
                    // ノートオン・オフ処理.
                    else if ((fileData[j] & 0xf0) == 0x90 || (fileData[j] & 0xf0) == 0x80) {
                        track[TrackPoint].event[eventPoint].datasize = 3;
                        track[TrackPoint].event[eventPoint].data = &fileData[j];
                        if ((fileData[j] & 0xf0) == 0x90) {
                            track[TrackPoint].event[eventPoint].eventtype = NOTE_ON;
                        } else if ((fileData[j] & 0xf0) == 0x80) {
                            track[TrackPoint].event[eventPoint].eventtype = NOTE_OFF;
                        }
                        j += 3;
                    }
                    // 3バイト使うイベント.
                    else if ((fileData[j] & 0xf0) == 0xA0 || (fileData[j] & 0xf0) == 0xB0 || (fileData[j] & 0xf0) == 0xE0) {
                        track[TrackPoint].event[eventPoint].datasize = 3;
                        track[TrackPoint].event[eventPoint].data = &fileData[j];
                        if ((fileData[j] & 0xf0) == 0xA0) {
                            track[TrackPoint].event[eventPoint].eventtype = POLYPHONIC_KEY_PRESSURE;
                        } else if ((fileData[j] & 0xf0) == 0xB0) {
                            track[TrackPoint].event[eventPoint].eventtype = CONTROL;
                        } else if ((fileData[j] & 0xf0) == 0xE0) {
                            track[TrackPoint].event[eventPoint].eventtype = PITCH_BENT;
                        }
                        j += 3;
                    }
                    // 2バイト使うイベント.
                    else if ((fileData[j] & 0xf0) == 0xC0 || (fileData[j] & 0xf0) == 0xD0) {
                        track[TrackPoint].event[eventPoint].datasize = 2;
                        track[TrackPoint].event[eventPoint].data = &fileData[j];
                        if ((fileData[j] & 0xf0) == 0xC0) {
                            track[TrackPoint].event[eventPoint].eventtype = PROGRAM_CHANGE;
                        } else if ((fileData[j] & 0xf0) == 0xD0) {
                            track[TrackPoint].event[eventPoint].eventtype = CHANNEL_PRESSURE;
                        }
                        j += 2;
                    }
                    else {
                        // それ以外は定義されていない.
                        printf("0x%02X は定義されていません\n", fileData[j]);
                        return -1;
                    }
                    eventPoint++;
                }
                delta = !delta;
                //printf("読み取り成功\n");
            }
            i += trackSize - 1;
            printf("トラック%d解析2週目完了\n", TrackPoint + 1);
            TrackPoint += 1;
        } else {
            // MTrkが最初に来ないのでエラー.
            printf("初期トラックヘッダ確認失敗\n");
            return -1;
        }
    }

    // fileDataを解放.
    free(fileData);
    printf("MIDIファイルの解析が完了しました\n");

    // トラックが3つ以上ある場合はトラック3以降のデータを無視するか聞く.
    while (maxTrackNum > 2) {
        printf("トラックが3つ以上ありますが、トラック3以降のデータは無視しますか？(y/n) : ");
        char ans;
        scanf("%c", &ans);
        if (ans == 'y' || ans == 'Y') {
            maxTrackNum = 2;
            break;
        } else if (ans == 'n' || ans == 'N') {
            printf("プログラムを終了します\n");
            return -1;
        } else {
            printf("yかnで答えてください\n");
        }
    }
    

    // 解析したデータを書き込む.
    FILE* midi_out = fopen("midi_out.txt", "w");
    if (midi_out == NULL) {
        printf("アウトプットファイルを開けませんでした\n");
        fclose(midi_out);
        return -1;
    } else {
        printf("アウトプットファイルを開きました\n");
    }

    

    // 元のファイルの名前をコメントで書き込む.
    fprintf(midi_out, "// 元のファイル名: %s\n", file_name);
    // フォーマットを書き込む.
    fprintf(midi_out, "// フォーマット: %d\n", format);
    // トラック数を書き込む.
    fprintf(midi_out, "// トラック数: %d\n", maxTrackNum);
    // 四分音符の長さを書き込む.
    fprintf(midi_out, "// 四分音符の長さ: %d\n", resolution);
    
    



    
    

    // プログラム出力ファイルを閉じる.
    fclose(midi_out);
    return 0;
}