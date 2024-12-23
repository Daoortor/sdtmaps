#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "test-data/minsk-subway.h"
#include "../core/include/pathfinder.h"
#include "../models/include/util.h"
#include "util.h"
#include "../core/include/substring-finder.h"

namespace sdtmaps::test {

TEST_CASE("Pathfinder test") {
    SUBCASE("Minsk subway") {
        TransportSystem minskSubway = makeMinskSubway();

        REQUIRE(minskSubway.isValid());

        SUBCASE("Trivial journey") {
            TestCase(minskSubway, "113", "113", QDateTime(minskSubwayDate, QTime(3, 30)), {}).check();
        }

        SUBCASE("Simple journey") {
            TestCase(
                minskSubway, "113", "114", QDateTime(minskSubwayDate, QTime(3, 30)),
                {
                    {
                        {JourneyElementType::RIDE, "113", "114"}
                    },
                    QDateTime(minskSubwayDate, QTime(4, 20))
            }).check();
        }

        SUBCASE("One change") {
            TestCase(
                minskSubway, "214", "119", QDateTime(minskSubwayDate, QTime(3, 30)),
                {
                    {
                        {JourneyElementType::RIDE, "214", "216"},
                        {JourneyElementType::TRANSFER, "216", "116"},
                        {JourneyElementType::RIDE, "116", "119"}
                    },
                    QDateTime(minskSubwayDate, QTime(4, 45))
                }
            );
        }
    }

    SUBCASE("Hamburg") {
        TransportSystem hamburg(QDir(PROJECT_ROOT_PATH "/data/gtfs_hamburg"));
        QDate hamburgDate(2020, 03, 20);

        REQUIRE(hamburg.isValid());

        SUBCASE("Iserbrook - Hamburg Hbf") {
            TestCase(
                hamburg, "000000081951", "000000010950", QDateTime(hamburgDate, QTime(6, 00)),
                {
                    {
                        {JourneyElementType::RIDE, "000000081951", "000000080953"},
                        {JourneyElementType::RIDE, "000000080953", "000000010950"}
                    },
                    QDateTime(hamburgDate, QTime(6, 31))
                }
            ).check();
        }

        SUBCASE("Bremen Hbf - Kiel Hbf") {
            TestCase(
                hamburg, "000008000050_G", "000008000199_G", QDateTime(hamburgDate, QTime(6, 00)),
                {
                    {
                        {JourneyElementType::RIDE, "000008000050_G", "000008000147_G"},
                        {JourneyElementType::RIDE, "000008000147_G", "000000010950"},
                        {JourneyElementType::RIDE, "000000010950", "000008000199_G"},
                    },
                    QDateTime(hamburgDate, QTime(9, 32))
                }
            ).check();
        }

        SUBCASE("Triangle inequality") {
            const int N_TESTS = 10;

            for (int i = 0; i < N_TESTS; i++) {
                std::uniform_int_distribution<> randomStop(0, hamburg.stops.size() - 1);
                const Stop *a = &hamburg.stops[randomStop(rng)];
                const Stop *b = &hamburg.stops[randomStop(rng)];
                const Stop *c = &hamburg.stops[randomStop(rng)];
                std::optional<Journey> aTob_ = pathfind(hamburg, a->id, b->id, initDateTime);
                if (!aTob_.has_value()) {
                    continue;
                }
                Journey &aTob = aTob_.value();
                std::optional<Journey> bToc_ = pathfind(hamburg, b->id, c->id, aTob.arrivalTime);
                if (!bToc_.has_value()) {
                    continue;
                }
                Journey &bToc = bToc_.value();
                std::optional<Journey> aToc_ = pathfind(hamburg, a->id, c->id, initDateTime);
                assert(aToc_.has_value());
                Journey &aToc = aToc_.value();
                assert(aToc.arrivalTime <= bToc.arrivalTime);
            }
        }
    }

    SUBCASE("Paris") {
        TransportSystem paris(QDir(PROJECT_ROOT_PATH "/data/gtfs_paris"));
        QDate parisDate(2021, 03, 25);

        REQUIRE(paris.isValid());

        SUBCASE("Temple - Pasteur") {
            TestCase(
                paris, "1683", "1781", QDateTime(parisDate, QTime(6, 00)),
                {
                    {
                        {JourneyElementType::RIDE, "1683", "1764"},
                        {JourneyElementType::RIDE, "1764", "1824"},
                        {JourneyElementType::RIDE, "1824", "1781"},
                    },
                    QDateTime(parisDate, QTime(6, 15))
                }
            ).check();
        }
    }
    
    SUBCASE("Milan") {
        TransportSystem milan(QDir(PROJECT_ROOT_PATH "/data/gtfs_milan"));
        QDate milanDate(2020, 03, 24);

        REQUIRE(milan.isValid());
    }
    
    SUBCASE("Stockholm") {
        TransportSystem stockholm(QDir(PROJECT_ROOT_PATH "/data/gtfs_stockholm"));
        QDate stockholmDate(2020, 03, 24);

        REQUIRE(stockholm.isValid());
    }
}

TEST_CASE("Substring find test") {
    SUBCASE("Simple") {
        std::vector strings = {QString("banana"), QString("bandana"), QString("anagram")};
        SuffixAutomaton sa(strings);

        QString query = "ana";
        std::vector<QString> result = sa.findAllStringsContaining(query);
        std::ranges::sort(result);

        std::vector expected = {QString("anagram"), QString("banana"), QString("bandana")};
        assert(expected == result);
    }

    SUBCASE("No matches") {
        std::vector strings = {QString("banana"), QString("bandana"), QString("anagram")};
        SuffixAutomaton sa(strings);

        QString query = "isomorphism";
        std::vector<QString> result = sa.findAllStringsContaining(query);

        std::vector<QString> expected;
        assert(expected == result);
    }

    SUBCASE("Stress test") {
        const int N_TESTS = 5;
        const int N_STRINGS = 50;
        const int N_SUBSTRINGS = 200;
        const int STRING_LENGTH = 10;
        const int SUBSTRING_LENGTH = 2;

        for (int i = 0; i < N_TESTS; i++) {
            std::vector strings = generateStrings(N_STRINGS, STRING_LENGTH);
            SuffixAutomaton sa(strings);
            for (int j = 0; j < N_SUBSTRINGS; j++) {
                QString substring = generateString(SUBSTRING_LENGTH);
                std::vector<QString> expected = naiveFindAllStringsContaining(strings, substring);
                std::vector<QString> actual = sa.findAllStringsContaining(substring);
                assert(expected == actual);
            }
        }
    }
}

}
