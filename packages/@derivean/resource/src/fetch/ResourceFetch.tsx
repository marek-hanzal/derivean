import {withFetch}         from "@use-pico/source-ui";
import {withResourceQuery} from "../query/withResourceQuery";

export const ResourceFetch = withFetch({withQuery: withResourceQuery});
