#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

class BookParser {
public:
    BookParser() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Curl initialization failed." << std::endl;
        }
    }

    ~BookParser() {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }

    void parseAndSaveBooks(const std::string& url, const std::string& outputFileName) {
        if (curl) {
            // Устанавливаем URL
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            // Устанавливаем User-Agent
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36");

            // Устанавливаем обработчик для записи данных
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            // Выполняем запрос
            CURLcode res = curl_easy_perform(curl);

            // Проверяем наличие ошибок
            if (res != CURLE_OK) {
                std::cerr << "Curl failed: " << curl_easy_strerror(res) << std::endl;
                return;
            }

            // Парсим HTML с использованием libxml
            parseBooks();

            // Сохраняем данные в файл
            saveToFile(outputFileName);
        }
        else {
            std::cerr << "Curl initialization failed." << std::endl;
        }
    }

private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        size_t totalSize = size * nmemb;
        output->append(reinterpret_cast<char*>(contents), totalSize);
        return totalSize;
    }

    void parseBooks() {
        htmlDocPtr doc = htmlReadDoc(reinterpret_cast<const xmlChar*>(response.c_str()), nullptr, nullptr, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
        if (doc) {
            // Создаем контекст XPath
            xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
            if (xpathCtx) {
                // Выполняем XPath запросы
                xpathQueryAndSave(xpathCtx, "//div[@class='product-title__head']", "title.txt");
                xpathQueryAndSave(xpathCtx, "//div[@class='product-title__author']", "author.txt");
                xpathQueryAndSave(xpathCtx, "//div[@class='product-price__value product-price__value--discount']", "price.txt");

                // Освобождаем контекст XPath
                xmlXPathFreeContext(xpathCtx);
            }

            // Освобождаем документ
            xmlFreeDoc(doc);
        }
    }

    void xpathQueryAndSave(xmlXPathContextPtr xpathCtx, const char* xpathExpr, const std::string& outputFileName) {
        xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(xpathExpr), xpathCtx);
        if (xpathObj) {
            std::ofstream outputFile(outputFileName);
            for (int i = 0; i < xpathObj->nodesetval->nodeNr; ++i) {
                xmlNodePtr node = xpathObj->nodesetval->nodeTab[i];
                std::string content = reinterpret_cast<const char*>(xmlNodeGetContent(node));
                outputFile << content << std::endl;
            }
            outputFile.close();

            // Освобождаем объект XPath
            xmlXPathFreeObject(xpathObj);
        }
    }

    void saveToFile(const std::string& outputFileName) {
        std::ofstream outputFile(outputFileName);
        outputFile << response;
        outputFile.close();
    }

    CURL* curl;
    std::string response;
};

int main() {
    BookParser bookParser;
    bookParser.parseAndSaveBooks("https://www.chitai-gorod.ru/catalog/books-18030/manga-110064", "output.txt");

    return 0;
}
