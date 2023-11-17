import {type IContainer}  from "../../api/IContainer";
import {RedisService}     from "../../service/cache/RedisService";
import {withCacheService} from "../service/withCacheService";

export const withCacheContainer: IContainer.Register = container => {
    withCacheService.bind(container, RedisService);
};
