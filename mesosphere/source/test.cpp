int main(void) {
    for(;;);
    return 0;
}

extern "C" {
    void _start(void) {
        main();
    }
}
