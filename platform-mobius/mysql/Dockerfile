FROM mysql:8.0.40-debian

RUN mkdir resources

WORKDIR "/resources"

COPY ./resources/init.sh ./mobiusdb.sql ./

RUN chmod 0555 ./init.sh && ./init.sh