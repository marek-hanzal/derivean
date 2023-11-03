import {withFetch}             from "@use-pico/source-ui";
import {withResourceTypeQuery} from "../query/type/withResourceTypeQuery";

export const ResourceTypeFetch = withFetch({withQuery: withResourceTypeQuery});
