import {
    type IContainer,
    withContainer
}                              from "@use-pico/container";
import {type IRpcHandler}      from "../api/IRpcHandler";
import {type IRpcIndexService} from "../api/IRpcIndexService";

export class RpcIndexService implements IRpcIndexService {
    static inject = [
        withContainer.inject,
    ];

    constructor(
        protected container: IContainer.Type,
    ) {
    }

    public register<T extends {
                                  new(...args: any): IRpcHandler<any, any>
                              } & {
                                  key: IContainer.Key
                              }>(handler: T): void {
        this.container.useClass(handler.key, handler);
    }

    public using<T extends {
                               new(...args: any): IRpcHandler<any, any>
                           } & {
                               key: IContainer.Key
                           }>(handlers: T[]): void {
        handlers.forEach(handler => this.register(handler));
    }
}
