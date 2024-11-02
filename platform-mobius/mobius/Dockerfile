FROM node:lts-jod

RUN mkdir -p /mobius_resources

COPY ./resources ./mobius_resources

WORKDIR /mobius_resources/

RUN chmod 0740 init.sh

ENTRYPOINT [ "./init.sh" ]
