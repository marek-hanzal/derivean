import {type IContainer}  from "@use-pico/container";
import {type IRpcHandler} from "./IRpcHandler";

export interface IRpcIndexService {
    register<T extends {
                           new(...args: any): IRpcHandler<any, any>
                       } & {
                           key: IContainer.Key
                       }>(handler: T): void;

    using<T extends {
                        new(...args: any): IRpcHandler<any, any>
                    } & {
                        key: IContainer.Key
                    }>(handlers: T[]): void;
}
