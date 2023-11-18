import {type Schema}      from "@use-pico/extras";
import {
    type CountSchema,
    type IWithMutation,
    type IWithQuery
}                         from "@use-pico/query";
import {type ArraySchema} from "@use-pico/schema";
import {type IContainer}  from "../api/IContainer";
import {type IHandler}    from "../api/IHandler";
import {type IRepository} from "../api/IRepository";
import {withCacheService} from "../container/service/withCacheService";
import {type withService} from "../service/withService";

export namespace withRepositoryHandler {
    export interface Props<
        TSchema extends Schema<any, any, any, any>,
        TWithRepository extends IRepository<any, TSchema, any>,
        TRepository extends new (...args: any) => TWithRepository,
        TQueryHandler extends IWithQuery<TSchema["query"], ArraySchema<TSchema["entity"]>>,
        TCountHandler extends IWithQuery<TSchema["query"], CountSchema>,
        TMutationHandler extends IWithMutation<TSchema["mutation"], TSchema["entity"]>,
    > {
        container: IContainer.Type;
        withRepository: withService.Service<TWithRepository>;
        repository: TRepository;
        handler: {
            query: TQueryHandler;
            count: TCountHandler;
            mutation: TMutationHandler;
        };
    }
}

export const withRepositoryHandler = <
    TSchema extends Schema<any, any, any, any>,
    TWithRepository extends IRepository<any, TSchema, any>,
    TRepository extends new (...args: any) => TWithRepository,
    TQueryHandler extends IWithQuery<TSchema["query"], ArraySchema<TSchema["entity"]>>,
    TCountHandler extends IWithQuery<TSchema["query"], CountSchema>,
    TMutationHandler extends IWithMutation<TSchema["mutation"], TSchema["entity"]>,
>(
    {
        container,
        withRepository,
        repository,
        handler: {
                     query,
                     count,
                     mutation,
                 },
    }: withRepositoryHandler.Props<
        TSchema,
        TWithRepository,
        TRepository,
        TQueryHandler,
        TCountHandler,
        TMutationHandler
    >
) => {
    withRepository.bind(container, repository);

    container.useValue<
        IHandler<
            typeof query.schema["request"],
            typeof query.schema["response"]
        >
    >(query.key.join("."), {
        schema: query.schema,
        handle: async ({
                           request,
                           container
                       }) => withRepository.use(container).withQuery.query(request),
    });

    container.useValue<
        IHandler<
            typeof count.schema["request"],
            typeof count.schema["response"]
        >
    >(count.key.join("."), {
        schema: count.schema,
        handle: async ({
                           request,
                           container
                       }) => withRepository.use(container).withQuery.count(request),
    });

    container.useValue<
        IHandler<
            typeof mutation.schema["request"],
            typeof mutation.schema["response"]
        >
    >(mutation.key.join("."), {
        schema: mutation.schema,
        cache:  {
            bypass: true,
        },
        handle: async ({
                           request,
                           container
                       }) => {
            await withCacheService.use(container).clear();
            return withRepository.use(container).withMutation.mutation(request);
        },
    });
};
