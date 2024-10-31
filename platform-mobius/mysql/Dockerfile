FROM mysql:8.0.40-debian

RUN mkdir resources

WORKDIR "/resources"

COPY ./resources/mobiusdb.sql /docker-entrypoint-initdb.d/


